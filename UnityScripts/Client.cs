using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

public class Client : MonoBehaviour {

	private Socket _clientSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
	private readonly byte[] _receiveBuffer = new byte[8142];
	public Dictionary<string, Vector3> pointsDictionary = new Dictionary<string, Vector3>();
	float _timeToGo;

	// Use this for initialization
	void Start ()
	{
		_clientSocket.BeginConnect(new IPEndPoint(IPAddress.Loopback, 27015), SetupClient, null);
		_timeToGo = Time.fixedTime + 1.0f;
	}

	// Update is called once per frame
	void Update() {
		if (!(Time.fixedTime >= _timeToGo)) return;
		if (!_clientSocket.Connected)
		{
			_clientSocket.Close();
			_clientSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			_clientSocket.BeginConnect(new IPEndPoint(IPAddress.Loopback, 27015), SetupClient, null);
			return;
		}
		SendData(Encoding.ASCII.GetBytes("send_points\0"));
		_clientSocket.BeginReceive(_receiveBuffer, 0, _receiveBuffer.Length, SocketFlags.None, ReceiveCallback, null);
		_timeToGo = Time.fixedTime + 0.1f;
	}


	void OnDestroy()
	{
		Debug.Log("Killing");
		if (_clientSocket.Connected)
			_clientSocket.Disconnect(true);
		_clientSocket.Shutdown(SocketShutdown.Both);
		_clientSocket.Close();
		Debug.Log("Killed");
	}

	private void SetupClient(IAsyncResult AR)
	{
		_clientSocket.EndConnect(AR);
		try
		{
			SendData(Encoding.ASCII.GetBytes("send_points\0"));
			_clientSocket.Blocking = false;
			_clientSocket.BeginReceive(_receiveBuffer, 0, _receiveBuffer.Length, SocketFlags.None, ReceiveCallback, null);
		}
		catch (SocketException ex)
		{
			Debug.Log(ex.Message);
		}
	}
	private void ReceiveCallback(IAsyncResult AR)
	{
		//Check how much bytes are recieved and call EndRecieve to finalize handshake
		int recieved = _clientSocket.EndReceive(AR);

		if (recieved <= 0)
			return;

		//Copy the recieved data into new buffer , to avoid null bytes
		byte[] recData = new byte[recieved];
		Buffer.BlockCopy(_receiveBuffer, 0, recData, 0, recieved);

		//Process data here the way you want , all your bytes will be stored in recData
		var recString = Encoding.Default.GetString(recData);

		var splittedStrings = recString.Split('\n');
		foreach (var splittedString in splittedStrings)
		{
			var nameOfPoint = splittedString.Split(':')[0];
			var coordStrings = splittedString.Split(':')[1].Split('/').Select(float.Parse).ToArray();

			pointsDictionary[nameOfPoint] = new Vector3(coordStrings[0], coordStrings[1], coordStrings[2]);
				foreach (KeyValuePair<string, Vector3> pair in pointsDictionary)
				{
					Debug.Log(pair.Key + ": Values: " + pair.Value);
				}
		}

		//Start receiving again
		//_clientSocket.BeginReceive(_receiveBuffer, 0, _receiveBuffer.Length, SocketFlags.None, ReceiveCallback, null);
	}

	private void SendData(byte[] data)
	{
		SocketAsyncEventArgs socketAsyncData = new SocketAsyncEventArgs();
		socketAsyncData.SetBuffer(data, 0, data.Length);
		_clientSocket.SendAsync(socketAsyncData);
	}
}