/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Devices.Client;
using Newtonsoft.Json;
using System;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace deviceSim
{
    class Program
    {
        static telemetry _telemetry=new telemetry();

        static DeviceClient client = null;
        static string connStr = ""; // device connection string (can be obtained from the Azure IoT Explorer)

        static async Task Main(string[] args)
        {
            if (string.IsNullOrEmpty(connStr))
            {
                Console.WriteLine("You need to set the device connection string 'connStr'");
                Console.WriteLine("This can be obtained from the Azure IoT Explorer tool, or from the Azure Portal (IoT Hub/Devices)");
                return;
            }

            client = DeviceClient.CreateFromConnectionString(connStr, Microsoft.Azure.Devices.Client.TransportType.Mqtt);

            _telemetry.temperature = 11.5;
            _telemetry.NoUpdateAvailable = false;
            _telemetry.AppRestart = false;

            int tickCounter = 0;
            while (true)
            {
                _telemetry.AppRestart = false;
                _telemetry.NoUpdateAvailable = false;

                if (tickCounter != 60 && tickCounter % 10 == 0)
                {
                    _telemetry.AppRestart = true;
                    Console.Write("*");
                }

                if (tickCounter == 60)
                {
                    _telemetry.NoUpdateAvailable = true;
                    tickCounter = 0;
                    Console.WriteLine("!");
                }

                if (!_telemetry.AppRestart && !_telemetry.NoUpdateAvailable)
                {
                    Console.Write(".");
                }

                tickCounter++;
                string telMsg = JsonConvert.SerializeObject(_telemetry);
                await SendTelemetryMessage(client, telMsg);
                Thread.Sleep(2000);
            }
        }

        static async Task SendTelemetryMessage(DeviceClient client, string message)
        {
            var msg = new Microsoft.Azure.Devices.Client.Message(Encoding.UTF8.GetBytes(message));
            msg.ContentEncoding = "utf-8";
            msg.ContentType = "application/json";
            await client.SendEventAsync(msg);
        }
    }

    public class telemetry
    {
        public double temperature { get; set; }
        public bool NoUpdateAvailable { get; set; }
        public bool AppRestart { get; set; }
    }

}
