using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace PanelPlayerExample
{
    class Program
    {
        static void ShowUsage()
        {
            Console.WriteLine("Usage:");
            Console.WriteLine("  PanelPlayerExample -p <port> -w <width> -h <height> [options] <sources>");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  -p <port>       Set ethernet port (required)");
            Console.WriteLine("  -w <width>      Set display width (required)");
            Console.WriteLine("  -h <height>     Set display height (required)");
            Console.WriteLine("  -b <brightness> Set display brightness (0-255, default: 255)");
            Console.WriteLine("  -m <mix>        Set frame mixing percentage (0-99, default: 0)");
            Console.WriteLine("  -r <rate>       Override source frame rate");
            Console.WriteLine("  -e <extension>  Load extension from file");
            Console.WriteLine("  -d              Duplicate each row vertically");
            Console.WriteLine("  -v              Enable verbose output");
            Console.WriteLine();
            Console.WriteLine("Sources:");
            Console.WriteLine("  One or more image/animation files (WebP, JPEG, PNG, GIF, BMP)");
        }

        static int Main(string[] args)
        {
            try
            {
                // Parse command-line arguments
                string port = null;
                int width = 0;
                int height = 0;
                int brightness = 255;
                int mix = 0;
                int rate = 0;
                string extension = null;
                bool duplicate = false;
                bool verbose = false;
                List<string> sources = new List<string>();

                for (int i = 0; i < args.Length; i++)
                {
                    if (args[i].StartsWith("-"))
                    {
                        switch (args[i])
                        {
                            case "-p":
                                if (++i >= args.Length)
                                {
                                    Console.WriteLine("Error: -p requires a value");
                                    ShowUsage();
                                    return 1;
                                }
                                port = args[i];
                                break;

                            case "-w":
                                if (++i >= args.Length || !int.TryParse(args[i], out width))
                                {
                                    Console.WriteLine("Error: -w requires an integer value");
                                    ShowUsage();
                                    return 1;
                                }
                                break;

                            case "-h":
                                if (++i >= args.Length || !int.TryParse(args[i], out height))
                                {
                                    Console.WriteLine("Error: -h requires an integer value");
                                    ShowUsage();
                                    return 1;
                                }
                                break;

                            case "-b":
                                if (++i >= args.Length || !int.TryParse(args[i], out brightness))
                                {
                                    Console.WriteLine("Error: -b requires an integer value");
                                    ShowUsage();
                                    return 1;
                                }
                                break;

                            case "-m":
                                if (++i >= args.Length || !int.TryParse(args[i], out mix))
                                {
                                    Console.WriteLine("Error: -m requires an integer value");
                                    ShowUsage();
                                    return 1;
                                }
                                break;

                            case "-r":
                                if (++i >= args.Length || !int.TryParse(args[i], out rate))
                                {
                                    Console.WriteLine("Error: -r requires an integer value");
                                    ShowUsage();
                                    return 1;
                                }
                                break;

                            case "-e":
                                if (++i >= args.Length)
                                {
                                    Console.WriteLine("Error: -e requires a value");
                                    ShowUsage();
                                    return 1;
                                }
                                extension = args[i];
                                break;

                            case "-d":
                                duplicate = true;
                                break;

                            case "-v":
                                verbose = true;
                                break;

                            default:
                                Console.WriteLine($"Error: Unknown option {args[i]}");
                                ShowUsage();
                                return 1;
                        }
                    }
                    else
                    {
                        sources.Add(args[i]);
                    }
                }

                // Validate required parameters
                if (port == null)
                {
                    Console.WriteLine("Error: Port must be specified!");
                    ShowUsage();
                    return 1;
                }

                if (width < 1 || height < 1)
                {
                    Console.WriteLine("Error: Width and height must be specified as positive integers!");
                    ShowUsage();
                    return 1;
                }

                if (brightness < 0 || brightness > 255)
                {
                    Console.WriteLine("Error: Brightness must be an integer between 0 and 255!");
                    ShowUsage();
                    return 1;
                }

                if (mix < 0 || mix >= 100)
                {
                    Console.WriteLine("Error: Mix must be an integer between 0 and 99!");
                    ShowUsage();
                    return 1;
                }

                if (sources.Count == 0)
                {
                    Console.WriteLine("Error: At least one source must be specified!");
                    ShowUsage();
                    return 1;
                }

                // Verify source files exist
                foreach (var source in sources)
                {
                    if (!File.Exists(source))
                    {
                        Console.WriteLine($"Error: Source file not found: {source}");
                        return 1;
                    }
                }

                if (verbose)
                {
                    Console.WriteLine($"Initializing PanelPlayer:");
                    Console.WriteLine($"  Port: {port}");
                    Console.WriteLine($"  Dimensions: {width}x{height}");
                    Console.WriteLine($"  Brightness: {brightness}");
                    if (mix > 0)
                        Console.WriteLine($"  Mix: {mix}%");
                    if (rate > 0)
                        Console.WriteLine($"  Frame Rate: {rate} fps");
                    if (duplicate)
                        Console.WriteLine($"  Duplicate: enabled");
                    if (extension != null)
                        Console.WriteLine($"  Extension: {extension}");
                    Console.WriteLine($"  Sources: {string.Join(", ", sources)}");
                    Console.WriteLine();
                }

                // Initialize PanelPlayer
                using (var player = new PanelPlayer(port, width, height, brightness))
                {
                    if (verbose)
                        Console.WriteLine("PanelPlayer initialized successfully!");

                    // Apply settings
                    if (mix > 0)
                    {
                        player.SetMix(mix);
                        if (verbose)
                            Console.WriteLine($"Frame mixing set to {mix}%");
                    }

                    if (rate > 0)
                    {
                        player.SetFrameRate(rate);
                        if (verbose)
                            Console.WriteLine($"Frame rate override set to {rate} fps");
                    }

                    if (duplicate)
                    {
                        player.SetDuplicate(true);
                        if (verbose)
                            Console.WriteLine("Duplicate mode enabled");
                    }

                    if (extension != null)
                    {
                        if (!File.Exists(extension))
                        {
                            Console.WriteLine($"Error: Extension file not found: {extension}");
                            return 1;
                        }
                        player.LoadExtension(extension);
                        if (verbose)
                            Console.WriteLine($"Extension loaded: {extension}");
                    }

                    // Play all source files
                    foreach (var source in sources)
                    {
                        if (verbose)
                            Console.WriteLine($"Playing: {source}");

                        player.PlayFile(source);

                        if (verbose)
                            Console.WriteLine($"Finished: {source}");
                    }

                    if (verbose)
                        Console.WriteLine("Playback completed successfully!");
                }

                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
                Console.WriteLine();
                Console.WriteLine("Note: This application requires root privileges and a valid ethernet interface.");
                Console.WriteLine("      Run with sudo and ensure the network interface is correct.");
                return 1;
            }
        }
    }
}
