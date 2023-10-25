using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;
using ThreadPriority = System.Threading.ThreadPriority;

public class MIDIOut : MonoBehaviour
{
    // Big mama toggle to turn MIDI out via network on or off
    public static bool MIDI_OUT_ACTIVE = false;

    private Thread midiSendThread;
    private Thread closeMessagesListener;

    private UdpClient udpClient;
    private static UdpClient udpSendClient;
    private static ConcurrentDictionary<string, string> connectedClients = new();
    private static ConcurrentDictionary<string, TcpClient> tcpClients = new();

    // Used to listen to beacons from the MIDI audio plugin
    private static readonly int UDP_LISTEN_PORT = 50000;
    // Used to send a response to the MIDI audio plugin
    private static readonly int UDP_RESPONSE_PORT = 50001;
    // Used to send MIDI messages to the MIDI audio plugin
    private static readonly int UDP_MIDI_SEND_PORT = 50002;
    // Used to keep track of connections through TCP
    private static readonly int TCP_CONNECTION_PORT = 50003;

    private readonly float beaconListenInterval = 0.5f;
    private static bool hasConnection = false;

    public static ConcurrentQueue<DataBuffer> inputQueue = new();

    public UnityEvent<bool> MIDIOutStatusChangedEvent = new();

    public enum MIDIMessageType {
        NOTE_ON = 0,
        NOTE_OFF = 1,
        PITCH_BEND = 2,
        CHANNEL_PRESSURE = 3,
        UNDEFINED = 4
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DataBuffer
    {
        public MIDIMessageType messageType;
        public int channel;
        public int note;
        public int velocity;
        public float bend; // Or aftertouch value
    }

    void Start()
    {
    }

    public void SendNoteOnMessage(int channel, int note, int velocity)
    {
        DataBuffer sendData = new DataBuffer {
            messageType = MIDIMessageType.NOTE_ON,
            channel = channel,
            note = note,
            velocity = velocity,
            bend = 0f
        };

        inputQueue.Enqueue(sendData);
    }

    public void SendNoteOffMessage(int note, int channel)
    {
        int velocity = 0;

        DataBuffer data = new DataBuffer {
            messageType = MIDIMessageType.NOTE_OFF,
            channel = channel,
            note = note,
            velocity = velocity,
            bend = 0f
        };

        inputQueue.Enqueue(data);
    }

    public void SendPitchBendMessage(int channel, float bendInSemitones)
    {
        DataBuffer data = new DataBuffer {
            messageType = MIDIMessageType.PITCH_BEND,
            channel = channel,
            note = 0,
            velocity = 0,
            bend = bendInSemitones
        };

        inputQueue.Enqueue(data);
    }

    public void SendChannelPressureMessage(int channel, float value)
    {
        DataBuffer data = new DataBuffer {
            messageType = MIDIMessageType.CHANNEL_PRESSURE,
            channel = channel,
            note = 0,
            velocity = 0,
            bend = value
        };

        inputQueue.Enqueue(data);
    }

    bool RegisterMidiReceiver(IPEndPoint ip)
    {
        // Add to list of connected clients
        if (connectedClients.ContainsKey(ip.Address.ToString()))
        {
            Debug.Log("Connection already established with " + ip.Address + ".");
            return false;
        }

        // Send Unity ack -> this will start make the MIDI audio plugin listen to the TCP connection
        // and start receiving MIDI messages
        Debug.Log("Sending ACK to " + ip.Address + " on port " + UDP_RESPONSE_PORT + ".");
        var ResponseData = Encoding.UTF8.GetBytes("NetzUnityAck");
        var IPEndpoint = new IPEndPoint(ip.Address, UDP_RESPONSE_PORT);
        udpClient.Send(ResponseData, ResponseData.Length, IPEndpoint);

        // Add TCP connection AFTER sending the UDP response (NetzUnityAck)
        try
        {
            var timeout = 3000;
            var client = new TcpClient();
            bool connected = TryTCPConnect(client, ip.Address.ToString(), TCP_CONNECTION_PORT, timeout); // 3-second timeout
            if (connected)
            {
                // Only return true if we successfully connected via TCP
                tcpClients[ip.Address.ToString()] = client;

                Debug.Log("Successfully TCP connected to " + ip.Address + " on port " + TCP_CONNECTION_PORT + ".");
                Debug.Log("Successfully registered MIDI receiver at " + ip.Address + ".");

                // Ugly but free concurrent
                connectedClients[ip.Address.ToString()] = ip.Address.ToString();
                hasConnection = true;

                MIDIOutStatusChangedEvent.Invoke(true);

                return true;
            }
            else
            {
                Debug.Log("TCP connection failed after " + timeout + "ms, retrying...");
            }
        } catch (SocketException e)
        {
            Debug.LogError(e);
            Debug.Log("Couldn't establish TCP connection, aborting.");
        }

        return false;
    }

    public bool TryTCPConnect(TcpClient client, string host, int port, int timeout)
    {
        IAsyncResult result = client.BeginConnect(host, port, null, null);
        bool success = result.AsyncWaitHandle.WaitOne(timeout, true);

        if (success)
        {
            client.EndConnect(result);
        }
        else
        {
            // Timeout occurred, abort the connection attempt
            client.Close();
        }

        return success;
    }

    private IEnumerator ListenToBeaconCoroutine()
    {
        while (MIDI_OUT_ACTIVE)
        {
            while (!hasConnection)
            {
                MIDIOutStatusChangedEvent.Invoke(false);
                var ClientEp = new IPEndPoint(IPAddress.Broadcast, UDP_LISTEN_PORT);
                try
                {
                    var netzMidiReceiverData = udpClient.Receive(ref ClientEp);
                    var dataStr = Encoding.UTF8.GetString(netzMidiReceiverData);
                    Debug.Log("Received " + dataStr + " from " + ClientEp.Address + ", sending response.");

                    if (dataStr.Contains("netz-midi-receiver"))
                    {
                        if (RegisterMidiReceiver(ClientEp))
                        {
                            // Success!
                            Debug.Log("Connection established with " + ClientEp.Address + ".");
                        }
                    }
                } catch (SocketException)
                {
                    Debug.Log("No connection yet...");
                }

                yield return new WaitForSeconds(beaconListenInterval);
            }
            yield return new WaitForSeconds(3.0f);
        }

    }

    // The method above but using a UDP client
    static void SendData(DataBuffer data) {
        byte[] bytes = new byte[Marshal.SizeOf(typeof(DataBuffer))];
        GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
        Marshal.StructureToPtr(data, handle.AddrOfPinnedObject(), false);
        handle.Free();
        foreach (var clientIP in connectedClients)
        {
            udpSendClient.Send(bytes, bytes.Length, clientIP.Key, UDP_MIDI_SEND_PORT);
        }
    }

    private void OnApplicationQuit()
    {
        Debug.Log("Closing connection to all clients.");

        ShutdownMIDIOut();
    }


    public class MIDISendThread
    {
        public static void Run()
        {
            while (MIDI_OUT_ACTIVE)
            {
                while(inputQueue.TryDequeue(out DataBuffer message))
                {
                    SendData(message);
                }
                Thread.Sleep(3);
            }
        }
    }

    public class CloseMessagesListener
    {
        public static void Run()
        {
            while (MIDI_OUT_ACTIVE)
            {
                List<string> ipsToRemove = new();

                // Check for data on our TCP clients
                foreach (var tcpClient in tcpClients)
                {

                    // Read data
                    var closeMessage = new byte[5];

                    try
                    {
                        String ip = tcpClient.Value.Client.RemoteEndPoint.ToString();
                        tcpClient.Value.GetStream().Read(closeMessage, 0, closeMessage.Length);
                        var dataStr = Encoding.UTF8.GetString(closeMessage);
                        ipsToRemove.Add(tcpClient.Key);
                        Debug.Log("Received close message '" + dataStr + "' from " + ip +
                                  ".");
                        Debug.Log("Closing TCP connection to " + ip + ".");
                    }
                    catch (Exception e)
                    {
                        // Don't need to do anything here, just means there is no data
                        Debug.Log("Exception while reading from TCP client: " + e);
                        Debug.Log("Note: this is normal as long as there are TCP clients connected.");
                        Debug.Log("If you get this while closing the MIDI receiver plug-in, you're doing something bad.");
                        Debug.LogError(e);
                    }
                }

                // Remove clients that have lost connection
                foreach (var ip in ipsToRemove)
                {
                    TcpClient removedClient;
                    Debug.Log("Removing client " + ip + " from list of connected TCP clients.");
                    tcpClients.TryRemove(ip, out removedClient);
                    connectedClients.TryRemove(ip, out _);
                    try
                    {
                        Debug.Log("Trying to close removed TCP socket.");
                        removedClient.GetStream().Close(100);
                        removedClient.Close();
                    } catch (Exception)
                    {
                        Debug.Log("Tried to close socket with " + ip + " but it was already closed.");
                    }

                    // Notify netz
                    Debug.Log("Connection to " + ip + " lost. Restarting beacon listener.");
                    Thread.Sleep(2000);

                    hasConnection = false;
                }
                Thread.Sleep(3000);
            }
        }
    }

    public void StartMIDIOut()
    {
        Debug.Log("Starting MIDI out...");
        MIDI_OUT_ACTIVE = true;
        udpClient = new UdpClient(UDP_LISTEN_PORT);
        udpClient.Client.ReceiveTimeout = 500;
        udpClient.Client.SendTimeout = 500;

        udpSendClient = new UdpClient();

        StartCoroutine(ListenToBeaconCoroutine());

        // Start MIDI message thread
        midiSendThread = new Thread(MIDISendThread.Run);
        midiSendThread.Start();

        // Start CloseMessagesListener thread
        closeMessagesListener = new Thread(CloseMessagesListener.Run);
        closeMessagesListener.Start();
    }

    /// <summary>
    /// Shuts down the whole MIDI out system
    /// </summary>
    public void ShutdownMIDIOut()
    {
        Debug.Log("Shutting down MIDI out...");

        MIDI_OUT_ACTIVE = false;
        StopAllCoroutines();

        // Go through all tcp clients and close them -> send a message to the MIDI audio plugin to stop listening
        foreach (var tcpClient in tcpClients)
        {
            try
            {
                Debug.Log("Closing TCP connection to " + tcpClient.Value.Client.RemoteEndPoint);
                tcpClient.Value.Close();
            } catch (Exception)
            {
                Debug.Log("Tried to close socket with " + tcpClient.Value.Client.RemoteEndPoint + " but it was already closed.");
            }
        }

        // Close all connections
        if (udpClient != null)
        {
            try
            {
                udpClient.Close();
            } catch (Exception)
            {
                Debug.Log("Tried to close UDP socket but it was already closed.");
            }
        }

        if (udpSendClient != null)
        {
            try
            {
                udpSendClient.Close();
            } catch (Exception)
            {
                Debug.Log("Tried to close UDP socket but it was already closed.");
            }
        }

        // Stop threads
        if(midiSendThread != null)
            midiSendThread.Join(300);
        if(closeMessagesListener != null)
            closeMessagesListener.Join(300);

        // Empty containers
        inputQueue.Clear();

        // Reset static variables
        connectedClients.Clear();
        tcpClients.Clear();

        hasConnection = false;
    }
}