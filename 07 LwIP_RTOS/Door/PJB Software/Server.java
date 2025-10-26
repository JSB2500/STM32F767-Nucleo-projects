
package uk.co.sologin.server;

import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Iterator;

public class Server
{
	private static final int WEB_PORT = 59467;
	private static final int ARDUINO_PORT = 18753;
	private static final int KEEPALIVE_TIMEOUT = 2000;
	private static final String expectedHeader = "GET /arduino HTTP/1.1\r\n\r\n";
	
	private Object arduinoLock = new Object();

	private ArrayList<Socket> potentialArduinoSockets = new ArrayList<Socket>();
	private Socket arduinoSocket;
	private boolean connected = false;
	private boolean doorOpen = false;

	private String webpage = "";

	public static void main(String[] args)
	{
		System.out.println("Starting servers...");
		new Server();
	}

	public Server()
	{
		new ArduinoServerThread().start();
		new WebServerThread().start();
		new ArduinoKeepAlive().start();

		try
		{
			InputStream in = getClass().getResourceAsStream("/uk/co/sologin/server/page/page.html");
			byte[] buffer = new byte[1024];
			int len = 0;
			while ((len = in.read(buffer)) > 0)
				webpage += new String(buffer, 0, len);
			in.close();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public class ArduinoServerThread extends Thread
	{
		private ServerSocket arduinoServer;

		public ArduinoServerThread()
		{
			while (arduinoServer == null)
			{
				try
				{
					arduinoServer = new ServerSocket(ARDUINO_PORT);
				}
				catch (IOException e)
				{
					System.err.println("Failed to start arduino server!");
					e.printStackTrace();
					try
					{
						arduinoServer.close();
					}
					catch (IOException e2)
					{
						e2.printStackTrace();
					}
					arduinoServer = null;
					try
					{
						Thread.sleep(1000);
					}
					catch (InterruptedException e1)
					{
						System.err.println("Failed to sleep!");
					}
				}
			}
		}

		@Override
		public void run()
		{
			System.out.println("Starting arduino server thread...");
			while (true)
			{
				try
				{
					Socket socket = arduinoServer.accept();
					potentialArduinoSockets.add(socket);
					checkForArduino();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}

	public void checkForArduino() throws IOException
	{
		if (connected)
			return;
		Iterator<Socket> iterator = potentialArduinoSockets.iterator();
		while (iterator.hasNext())
		{
			Socket socket = iterator.next();
			try
			{
				ReadWithTimeout readHeader = new ReadWithTimeout(socket.getInputStream(), expectedHeader.getBytes().length, KEEPALIVE_TIMEOUT);
				readHeader.start();
				String header = new String(readHeader.getData());
				if (header.equals(expectedHeader))
				{
					System.out.println("Arduino connected!");
					arduinoSocket = socket;
					connected = true;
				}
			}
			catch (IOException e)
			{
				socket.close();
			}
			finally
			{
				iterator.remove();
			}
		}
	}

	public class ArduinoKeepAlive extends Thread
	{
		@Override
		public void run()
		{
			while (true)
			{
				while (connected)
				{
					try
					{
						synchronized (arduinoLock)
						{
							arduinoSocket.getOutputStream().write('p');
							ReadWithTimeout readChar = new ReadWithTimeout(arduinoSocket.getInputStream(), 1, KEEPALIVE_TIMEOUT);
							readChar.start();
							byte[] data = readChar.getData();
							if (data[0] != 'o' && data[0] != 'c')
								throw new RuntimeException();
							doorOpen = data[0] == 'o';
						}
					}
					catch (Throwable e1)
					{
						connected = false;
						try
						{
							arduinoSocket.close();
						}
						catch (IOException e)
						{
							e.printStackTrace();
						}
						System.out.println("Arduino disconnected!");
					}
					try
					{
						Thread.sleep(200);
					}
					catch (InterruptedException e)
					{
						e.printStackTrace();
					}
				}
				try
				{
					Thread.sleep(10);
				}
				catch (InterruptedException e)
				{
					e.printStackTrace();
				}
			}
		}
	}

	public class ReadWithTimeout extends Thread
	{
		private InputStream in;
		private byte[] data;
		private int timeout;

		public ReadWithTimeout(InputStream in, int bufferSize, int timeout)
		{
			this.in = in;
			data = new byte[bufferSize];
			this.timeout = timeout;
		}

		@Override
		public void run()
		{
			try
			{
				in.read(data);
			}
			catch (IOException e)
			{
			}
		}

		public byte[] getData()
		{
			try
			{
				join(timeout);
			}
			catch (InterruptedException e)
			{
			}
			return data;
		}
	}

	public class WebServerThread extends Thread
	{
		private ServerSocket webServer;

		public WebServerThread()
		{
			try
			{
				webServer = new ServerSocket(WEB_PORT);
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}

		@Override
		public void run()
		{
			System.out.println("Starting web server thread...");
			while (true)
			{
				try
				{
					Socket socket = webServer.accept();
					new ClientHandler(socket).start();
				}
				catch (Exception e)
				{
					e.printStackTrace();
					break;
				}
			}
			try
			{
				webServer.close();
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
			System.out.println("Web server closed!");
		}
	}

	private class ClientHandler extends Thread
	{
		private Socket socket;

		public ClientHandler(Socket socket)
		{
			this.socket = socket;
		}

		@Override
		public void run()
		{
			try
			{
				InputStream in = socket.getInputStream();
				String request = "";
				int c;
				char preChar = 0;
				while ((c = in.read()) != -1)
				{
					request += (char) c;
					if ((char) c == '\r' && preChar == '\n')
						break;
					preChar = (char) c;
				}
				String[] headerLines = request.split("\n");
				String pageRequest = "";
				for (String line : headerLines)
				{
					if (line.startsWith("GET") || line.startsWith("POST"))
					{
						pageRequest = line.substring(line.indexOf(" ") + 1, line.indexOf(" ", line.indexOf(" ") + 1));
						break;
					}
				}
				
				PrintStream out = new PrintStream(socket.getOutputStream());
				if (pageRequest.equals("/status"))
				{
					out.print("HTTP/1.1 200 OK\r\n");
					out.print("Content-Type: text/plain; charset=UTF-8\r\n");
					out.print("Connection: close\r\n");
					String response = "";
					if (connected)
					{
						response = doorOpen ? "Close door" : "Open door";
						response += "\n" + (doorOpen ? "Door is currently open." : "Door is currently closed.");
					}
					else
					{
						response = "Waiting...";
						response += "\n" + "Waiting for arduino!<br>Try again shortly...";
					}
					out.print("Content-Length: " + response.getBytes().length + "\r\n\r\n");
					out.print(response);
				}
				else if (pageRequest.equals("/toggle_door") && connected)
				{
					in.read();
					String md5 = "";
					while ((c = in.read()) != -1)
						md5 += (char) c;
					if (md5.equals("46bf36a7193438f81fccc9c4bcc8343e"))
					{
						synchronized (arduinoLock)
						{
							arduinoSocket.getOutputStream().write('o');
							try
							{
								Thread.sleep(50);
								doorOpen = !doorOpen;
							}
							catch (Exception e) {}
						}
					}
					out.print("HTTP/1.1 200 OK\r\n");
					out.print("Content-Type: text/plain; charset=UTF-8\r\n");
					out.print("Connection: close\r\n");
					String buttonText = doorOpen ? "Close door" : "Open door"; 
					out.print("Content-Length: " + buttonText.getBytes().length + "\r\n\r\n");
					out.print(buttonText);
				}
				else
				{
					String page;
					if (connected)
					{
						page = webpage.replaceFirst("%s", doorOpen ? "Door is currently open." : "Door is currently closed.");
						String buttonText = "<button id=\"button\" type=\"submit\" class=\"btn btn-primary\" onclick=\"toggleDoor()\">%s</button>";
						page = page.replaceFirst("%s", doorOpen ? buttonText.replace("%s", "Close door") : buttonText.replace("%s", "Open door"));
					}
					else
					{
						page = webpage.replaceFirst("%s", "Waiting for arduino!<br>Try again shortly...");
						page = page.replaceFirst("%s", "<button id=\"button\" type=\"submit\" class=\"btn btn-primary\" onclick=\"toggleDoor()\">Waiting...</button>");
					}
					out.print("HTTP/1.1 200 OK\r\n");
					out.print("Content-Type: text/html; charset=UTF-8\r\n");
					out.print("Connection: close\r\n");
					out.print("Content-Length: " + page.getBytes().length + "\r\n\r\n");
					out.print(page);
				}
				out.flush();
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			finally
			{
				try
				{
					Thread.sleep(250);
					socket.close();
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		}
	}
}

