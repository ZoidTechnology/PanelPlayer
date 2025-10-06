using System;
using System.Runtime.InteropServices;

namespace PanelPlayerExample
{
    /// <summary>
    /// Native P/Invoke declarations for libpanelplayer.so
    /// </summary>
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
        public static extern int panelplayer_set_duplicate(
            [MarshalAs(UnmanagedType.I1)] bool enable
        );

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
}
