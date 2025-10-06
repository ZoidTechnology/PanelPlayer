using System;
using System.IO;

namespace PanelPlayerExample
{
    /// <summary>
    /// Managed wrapper for PanelPlayer native library
    /// </summary>
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

        public void SetDuplicate(bool enable)
        {
            int result = PanelPlayerNative.panelplayer_set_duplicate(enable);
            if (result != PanelPlayerNative.PANELPLAYER_SUCCESS)
            {
                throw new Exception($"Failed to set duplicate mode: {result}");
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
}
