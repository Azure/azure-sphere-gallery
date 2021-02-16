/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace PcUdpLogReceiver
{
    class Program
    {
        private const int listenPort = 1824;
        private static ConsoleColor Concolor = ConsoleColor.Gray;

        static void Main(string[] args)
        {
            Console.Clear();
            Console.ForegroundColor = ConsoleColor.White;

            bool showDateTime = true;
            uint DeviceHash = 0xffffffff;
            if (args.Length == 1)       // see if we have a hex value for a device hash to show.
            {
                DeviceHash=(uint) Convert.ToUInt32(args[0], 16);
                Debug.WriteLine($"Device Hash: 0x{DeviceHash:X}");
            }

            UdpClient listener = new UdpClient(listenPort);
            IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);

            string outFile = Path.Combine(Directory.GetCurrentDirectory(), "deviceLog.txt");

            Console.WriteLine("Azure Sphere Console Debug Client");

            try
            {
                while (true)
                {
                    byte[] bytes = listener.Receive(ref groupEP);

                    if (bytes.Length > 4)
                    {
                        // build device Id.
                        uint rxDeviceId = (uint)((bytes[0] * 0x1000000) + (bytes[1] * 0x10000) + (bytes[2] * 0x100) + bytes[3]);

                        if (DeviceHash == 0xffffffff || (DeviceHash == rxDeviceId))
                        {

                            // Encoding.ASCII.GetString(bytes, 0, bytes.Length);
                            string output = Encoding.UTF8.GetString(bytes, 4, bytes.Length - 4);

                            Concolor = ConsoleColor.Gray;

                            if (output.Trim().ToLower().StartsWith("information:") || output.Trim().ToLower().StartsWith("info:"))
                            {
                                Concolor = ConsoleColor.Cyan;
                            }

                            if (output.Trim().ToLower().StartsWith("error:") || output.Trim().ToLower().StartsWith("warning:"))
                            {
                                Concolor = ConsoleColor.Red;
                            }

                            if (Concolor != ConsoleColor.Gray)
                            {
                                Console.ForegroundColor = Concolor;
                            }

                            if (DeviceHash == 0xffffffff)
                            {

                                Console.Write($"{rxDeviceId,0:X8} ");
                            }

                            if (showDateTime)
                            {
                                DateTime dt = DateTime.Now;
                                Console.Write($"{dt.ToShortDateString()} {dt.ToShortTimeString()}: ");
                                Debug.Write($"{dt.ToShortDateString()} {dt.ToShortTimeString()}: ");

                                string dateString = string.Format("{0} {1}:", dt.ToShortDateString(), dt.ToShortTimeString());
                                File.AppendAllText(outFile, dateString + Environment.NewLine);
                            }

                            Console.Write($"{output}");
                            Debug.Write($"{output}");
                            File.AppendAllText(outFile, output);

                            if (!output.Contains('\n'))
                            {
                                Console.WriteLine();
                                Debug.WriteLine("");
                                File.AppendAllText(outFile, Environment.NewLine);
                            }

                            if (Concolor != ConsoleColor.Gray)
                            {
                                Console.ForegroundColor = ConsoleColor.Gray;
                            }
                        }
                    }
                    else
                    {
                        Debug.WriteLine($"Rx'd buffer {bytes.Length} bytes.");
                    }
                }
            }
            catch (SocketException e)
            {
                Console.WriteLine(e);
            }
            finally
            {
                listener.Close();
            }
        }
    }
}
