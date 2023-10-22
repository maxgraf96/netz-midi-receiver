using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

public class MIDIOut : MonoBehaviour
{
    private UdpClient listener;
    private UdpClient sender;
    private List<string> connectedClients = new();
    
    void Start()
    {
        listener = new UdpClient(12345);
        sender = new UdpClient(12346);
        listener.Client.ReceiveTimeout = 500;
        listener.Client.SendTimeout = 500;
        sender.Client.ReceiveTimeout = 500;
        sender.Client.SendTimeout = 500;
        StartCoroutine(ListenToBeaconCoroutine());
    }

    private IEnumerator ListenToBeaconCoroutine()
    {
        var ResponseData = Encoding.UTF8.GetBytes("SomeResponseData");
        var ClientEp = new IPEndPoint(IPAddress.Broadcast, 12345);

        while (true)
        {
            try
            {
                var ClientRequestData = listener.Receive(ref ClientEp);
                var ClientRequest = Encoding.UTF8.GetString(ClientRequestData);

                Debug.Log("Received " + ClientRequest + " from " + ClientEp.Address.ToString() + ", sending response");
                var IPEndpoint = new IPEndPoint(ClientEp.Address, 12346);
                sender.Send(ResponseData, ResponseData.Length, IPEndpoint);
            } catch (SocketException e)
            {
                if (e.SocketErrorCode != SocketError.TimedOut)
                {
                    Debug.LogError(e);
                }
            }

            yield return new WaitForSeconds(0.1f);
        }
    }

}
