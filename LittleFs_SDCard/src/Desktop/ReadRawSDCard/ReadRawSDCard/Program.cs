using Microsoft.Win32.SafeHandles;
using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ReadRawSDCard
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("app needs:");
                Console.WriteLine("1. Physical drive name, use powershell 'wmic diskdrive list brief' to get the list");
                Console.WriteLine("2. Number of 512 byte blocks to read");
                Console.WriteLine();
                Console.WriteLine(@"Example: ReadRawSDCard \\.\PHYSICALDRIVE3 8192");
                Console.Write(">");
                Console.ReadLine();
                return;
            }

            ReadDiskData(args[0], Convert.ToUInt32(args[1]));
        }

        public enum EMoveMethod : uint
        {
            Begin = 0,
            Current = 1,
            End = 2
        }

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        static extern uint SetFilePointer(
            [In] SafeFileHandle hFile,
            [In] int lDistanceToMove,
            [Out] out int lpDistanceToMoveHigh,
            [In] EMoveMethod dwMoveMethod);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern SafeFileHandle CreateFile(string lpFileName, uint dwDesiredAccess,
          uint dwShareMode, IntPtr lpSecurityAttributes, uint dwCreationDisposition,
          uint dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport("kernel32", SetLastError = true)]
        internal extern static int ReadFile(SafeFileHandle handle, byte[] bytes,
           int numBytesToRead, out int numBytesRead, IntPtr overlapped_MustBeZero);

        static void ReadDiskData(string physicalDrive, UInt32 numberOfBlocks)
        {
            string outFile=Path.Combine(Environment.CurrentDirectory, "diskImage.bin");
            if (File.Exists(outFile))
            {
                File.Delete(outFile);
            }

            uint GENERIC_READ = 0x80000000;
            uint OPEN_EXISTING = 3;

            SafeFileHandle handleValue = CreateFile(physicalDrive, GENERIC_READ, 0, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
            if (handleValue.IsInvalid)
            {
                Console.WriteLine($"Failed to open '{physicalDrive}' - try running as administrator");
                return;
            }

            Console.WriteLine($"Opening output file '{outFile}'");
            FileStream myStream = File.OpenWrite(outFile);
            // allocate 'block size' for reading.
            byte[] buf = new byte[512];
            int moveToHigh = 0;
            int bytesRead = 0;

            for(UInt32 x=0; x < numberOfBlocks;x++)
            {
                Console.Write($"Block {x} of {numberOfBlocks}\r");
                int offset = Convert.ToInt32(x) * 512;
                SetFilePointer(handleValue, offset, out moveToHigh, EMoveMethod.Begin);
                ReadFile(handleValue, buf, 512, out bytesRead, IntPtr.Zero);
                myStream.Write(buf, 0, 512);
            }

            Console.WriteLine();

            myStream.Flush();
            myStream.Close();
            handleValue.Close();
        }
    }
}
