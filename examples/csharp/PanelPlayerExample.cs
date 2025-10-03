using System;
using System.Runtime.InteropServices;
using System.IO;

namespace PanelPlayerExample
{
    public static class PanelPlayerNative
    {
        private const string LibraryName = "libpanelplayer.so";
        
        public const int PANELPLAYER_SUCCESS = 0;
        public const int PANELPLAYER_ERROR = -1;
        public const int PANELPLAYER_INVALID_PARAM = -2;
        public const int PANELPLAYER_NOT_INITIALIZED = -3;
        public const int PANELPLAYER_ALREADY_INITIALIZED = -4;

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_init(
            [MarshalAs(UnmanagedType.LPStr)] string interface_name,
            int width,
            int height,
            int brightness
        );

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_set_mix(int mix_percentage);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_set_rate(int frame_rate);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_load_extension(
            [MarshalAs(UnmanagedType.LPStr)] string extension_path
        );

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_play_file(
            [MarshalAs(UnmanagedType.LPStr)] string file_path
        );

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_play_frame_bgr(
            [MarshalAs(UnmanagedType.LPArray)] byte[] bgr_data,
            int width,
            int height
        );

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int panelplayer_send_brightness(
            byte red,
            byte green,
            byte blue
        );

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool panelplayer_is_initialized();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void panelplayer_cleanup();
    }

    public class PanelPlayer : IDisposable
    {
        private bool disposed = false;

        public PanelPlayer(string interfaceName, int width, int height, int brightness = 255)
        {
            int result = PanelPlayerNative.panelplayer_init(interfaceName, width, height, brightness);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to initialize PanelPlayer: {result}");
            }
        }

        public void SetMix(int mixPercentage)
        {
            int result = PanelPlayerNative.panelplayer_set_mix(mixPercentage);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to set mix: {result}");
            }
        }

        public void SetFrameRate(int frameRate)
        {
            int result = PanelPlayerNative.panelplayer_set_rate(frameRate);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to set frame rate: {result}");
            }
        }

        public void LoadExtension(string extensionPath)
        {
            int result = PanelPlayerNative.panelplayer_load_extension(extensionPath);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to load extension: {result}");
            }
        }

        public void PlayFile(string filePath)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException($"Image file not found: {filePath}");
            }

            int result = PanelPlayerNative.panelplayer_play_file(filePath);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to play image file: {result}");
            }
        }

        public void PlayFrameBGR(byte[] bgrData, int width, int height)
        {
            if (bgrData == null)
            {
                throw new ArgumentNullException(nameof(bgrData));
            }

            if (bgrData.Length != width * height * 3)
            {
                throw new ArgumentException("BGR data length doesn't match width * height * 3");
            }

            int result = PanelPlayerNative.panelplayer_play_frame_bgr(bgrData, width, height);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to play frame: {result}");
            }
        }

        public void SetBrightness(byte red, byte green, byte blue)
        {
            int result = PanelPlayerNative.panelplayer_send_brightness(red, green, blue);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to set brightness: {result}");
            }
        }

        public bool IsInitialized => PanelPlayerNative.panelplayer_is_initialized();

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposed)
            {
                PanelPlayerNative.panelplayer_cleanup();
                disposed = true;
            }
        }

        ~PanelPlayer()
        {
            Dispose(false);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {

            try
            {
                string interfaceName = "end0"; // Network interface (usually eth0)
                int width = 192;
                int height = 64;
                int brightness = 255;
                // Basic use example (Ejemplo básico de uso)
                using (var player = new PanelPlayer(interfaceName, width, height, brightness))
                {
                    Console.WriteLine("PanelPlayer initialized successfully!");

                    // Play image file (Supports WebP, JPEG, PNG, GIF, BMP)
                    if (args.Length > 0)
                    {
                        player.PlayFile(args[0]);
                        Console.WriteLine($"Played image file: {args[0]}");
                        System.Threading.Thread.Sleep(10000); // Wait for 10 seconds
                    }
                    
                    // Manualy send blue frame (Ejemplo de frame manual (cuadrado azul))
                    byte[] blueFrame = new byte[width * height * 3];
                    for (int i = 0; i < blueFrame.Length; i += 3)
                    {
                        blueFrame[i] = 255;     // Blue
                        blueFrame[i + 1] = 0;   // Green
                        blueFrame[i + 2] = 0;   // Red
                    }
                    
                    player.PlayFrameBGR(blueFrame, width, height);
                    Console.WriteLine("Displayed blue frame");
                    
                    // Wait before exit. (Esperar un poco antes de salir)
                    System.Threading.Thread.Sleep(2000);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
                Console.WriteLine("Note: This example requires root privileges and a valid ethernet interface.");
            }
        }
    }
}