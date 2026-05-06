import serial
import pygame
import sys
import os
import threading

current_music_type = None

def play_music(music_type):
    global current_music_type
    pygame.mixer.init()
    try:
        # Stop current music if any
        if pygame.mixer.get_init() and pygame.mixer.music.get_busy():
            pygame.mixer.music.stop()

        current_music_type = music_type

        if music_type == "EPILOGUE" and os.path.exists('epilogue.flac'):
            pygame.mixer.music.load('epilogue.flac')
        elif music_type == "BGM" and os.path.exists('bgm.flac'):
            pygame.mixer.music.load('bgm.flac')
        elif music_type == "CG" and os.path.exists('cg_music.flac'):
            pygame.mixer.music.load('cg_music.flac')
        else:
            # Fallback to whatever is available
            if os.path.exists('epilogue.flac'):
                pygame.mixer.music.load('epilogue.flac')
            elif os.path.exists('bgm.flac'):
                pygame.mixer.music.load('bgm.flac')
            elif os.path.exists('cg_music.flac'):
                pygame.mixer.music.load('cg_music.flac')
            else:
                print("Error: Could not find 'epilogue.flac', 'bgm.flac' or 'cg_music.flac' in the current directory.")
                return

        if music_type == "BGM":
            pygame.mixer.music.play(0) # Play BGM only once
        else:
            pygame.mixer.music.play(-1) # Loop indefinitely for others
        print("Music started.")
    except Exception as e:
        print(f"Error loading or playing music: {e}")

def stop_music(force=False):
    global current_music_type
    if current_music_type == "EPILOGUE" and not force:
        print("Ignored stop command because EPILOGUE is playing.")
        return

    if pygame.mixer.get_init():
        pygame.mixer.music.stop()
        print("Music stopped.")
        current_music_type = None

def main():
    if len(sys.argv) < 2:
        print("Usage: python play_music_serial.py <COM_PORT>")
        sys.argv.append("COM3") # Set default port
        print("Defaulting to COM3")

    port = sys.argv[1]
    baudrate = 115200

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Listening on {port} at {baudrate} baud...")
    except serial.SerialException as e:
        print(f"Failed to open serial port {port}: {e}")
        return

    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(f"Received: {line}")
                    if "PLAY_EPILOGUE_MUSIC" in line or "MUSIC:PLAY_EPILOGUE" in line:
                        threading.Thread(target=play_music, args=("EPILOGUE",)).start()
                    elif "PLAY_CG_MUSIC" in line:
                        threading.Thread(target=play_music, args=("CG",)).start()
                    elif "PLAY_BGM" in line:
                        threading.Thread(target=play_music, args=("BGM",)).start()
                    elif "STOP_CG_MUSIC" in line or "STOP_BGM_ONLY" in line or "STOP_BGM" in line or "STOP_EPILOGUE_MUSIC" in line or "STOP_MUSIC" in line or "MUSIC:STOP_EPILOGUE" in line:
                        stop_music()
        except KeyboardInterrupt:
            print("\nExiting...")
            stop_music(force=True)
            ser.close()
            break
        except Exception as e:
            print(f"Error reading serial: {e}")
            break

if __name__ == "__main__":
    main()
