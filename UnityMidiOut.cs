using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public class MIDIOut : MonoBehaviour
{
    private UdpClient udpClient;
    private Dictionary<string, TcpClient> connectedClients = new();

    private static readonly int BROADCAST_LISTEN_PORT = 12345;
    private static readonly int SEND_PORT = 12346;

    private float beaconListenInterval = 0.1f;

    private TcpClient tcpClient;

    public enum MIDIType
    {
        NoteOn = 1,
        NoteOff,
        ControlChange,
    }
    void Start()
    {
        Application.targetFrameRate = 60;

        udpClient = new UdpClient(BROADCAST_LISTEN_PORT);
        udpClient.Client.ReceiveTimeout = 500;
        udpClient.Client.SendTimeout = 500;
        StartCoroutine(ListenToBeaconCoroutine());
    }

    private void SendNoteOnMessage(int note, int velocity)
    {
        Debug.Log("Sending note on message to all connected clients: " + note + " " + velocity);

        int channel = 1;
        int[] integers = {channel, note, velocity};
        byte[] data = new byte[integers.Length * sizeof(int)];

        BitConverter.GetBytes(channel).CopyTo(data, 0);
        BitConverter.GetBytes(note).CopyTo(data, sizeof(int));
        BitConverter.GetBytes(velocity).CopyTo(data, sizeof(int) * 2);

        // Send note on message to all connected clients
        tcpClient.GetStream().Write(data, 0, data.Length);
        foreach (var client in connectedClients)
        {
        }
    }

    private void SendNoteOffMessage(int note)
    {
        Debug.Log("Sending note off message to all connected clients: " + note);

        int channel = 1;
        int velocity = 0;
        int[] integers = {channel, note, velocity};
        byte[] data = new byte[integers.Length * sizeof(int)];

        BitConverter.GetBytes(channel).CopyTo(data, 0);
        BitConverter.GetBytes(note).CopyTo(data, sizeof(int));
        BitConverter.GetBytes(velocity).CopyTo(data, sizeof(int) * 2);

        // Send note off message to all connected clients
        tcpClient.GetStream().Write(data, 0, data.Length);

        foreach (var client in connectedClients)
        {
        }
    }

    void RegisterMidiReceiver(string ip)
    {
        // Add to list of connected clients
        if(!connectedClients.TryAdd(ip, tcpClient))
            return;

        tcpClient = new TcpClient(ip, 12347);

        Debug.Log("Successfully registered MIDI receiver at " + ip);

        // Increase beacon listen interval, we already got a connection
        beaconListenInterval = 3.0f;
        // }
    }

    private IEnumerator ListenToBeaconCoroutine()
    {
        var ResponseData = Encoding.UTF8.GetBytes("SomeResponseData");
        var ClientEp = new IPEndPoint(IPAddress.Broadcast, BROADCAST_LISTEN_PORT);

        while (true)
        {
            try
            {
                var netzMidiReceiverData = udpClient.Receive(ref ClientEp);
                var dataStr = Encoding.UTF8.GetString(netzMidiReceiverData);
                Debug.Log("Received " + dataStr + " from " + ClientEp.Address + ", sending response.");

                var IPEndpoint = new IPEndPoint(ClientEp.Address, SEND_PORT);
                udpClient.Send(ResponseData, ResponseData.Length, IPEndpoint);

                if (dataStr.Contains("netz-midi-receiver"))
                {
                    RegisterMidiReceiver(ClientEp.Address.ToString());
                }
            } catch (SocketException e)
            {
                if (e.SocketErrorCode != SocketError.TimedOut)
                {
                    Debug.LogError(e);
                }
            }

            yield return new WaitForSeconds(beaconListenInterval);
        }
    }

    private void Update()
    {
        if(connectedClients.Count == 0)
            return;

        // Listen to keyboard input
        if (Input.GetKeyDown(KeyCode.A))
        {
            SendNoteOnMessage(60, 80);
        }

        if (Input.GetKeyUp(KeyCode.A))
        {
            SendNoteOffMessage(60);
        }

        // Same for s, d, f keys, chromatic scale
        if (Input.GetKeyDown(KeyCode.S))
        {
            SendNoteOnMessage(62, 80);
        }

        if (Input.GetKeyUp(KeyCode.S))
        {
            SendNoteOffMessage(62);
        }

        if (Input.GetKeyDown(KeyCode.D))
        {
            SendNoteOnMessage(64, 80);
        }

        if (Input.GetKeyUp(KeyCode.D))
        {
            SendNoteOffMessage(64);
        }

        if (Input.GetKeyDown(KeyCode.F))
        {
            SendNoteOnMessage(65, 80);
        }

        if (Input.GetKeyUp(KeyCode.F))
        {
            SendNoteOffMessage(65);
        }
    }

    private void OnApplicationQuit()
    {
        Debug.Log("Closing connection to all clients.");
        // foreach (var client in connectedClients)
        // {
            // client.Value.GetStream().Close();
            // client.Value.Close();
        // }

        // Close connection to client
        tcpClient.GetStream().Close();
        tcpClient.Close();
    }
}