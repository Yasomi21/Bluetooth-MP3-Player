import tkinter as tk
from tkinter import filedialog
import pygame
import os

import sys
import asyncio
import threading
from enum import Enum

from event_handler import EventHandler
from events import Events
# =============================================
#                   CONSTANTS
# =============================================
ADAFRUIT_BLE_MAC_ADDR = "C0:9E:48:AC:35:03"
MAX_PACKET_QUEUE_SIZE = 16

player = None
# =============================================
#                     MAIN
# =============================================
class MusicPlayer:
    def __init__(self, master):
        self.master = master
        master.title("Music Player")
        self.current_song = ""
        self.song_index = 0
        self.playlist = []
        self.paused = False

        # Initialize pygame mixer
        pygame.mixer.init()

        # Buttons
        self.load_button = tk.Button(master, text="Load Songs", command=self.load_music)
        self.load_button.pack()

        self.play_button = tk.Button(master, text="Play", command=self.toggle_play_pause)
        self.play_button.pack()

        self.stop_button = tk.Button(master, text="Stop", command=self.stop_music)
        self.stop_button.pack()

        self.next_button = tk.Button(master, text="Next", command=self.next_song)
        self.next_button.pack()

        self.previous_button = tk.Button(master, text="Previous", command=self.previous_song)
        self.previous_button.pack()

    def load_music(self):
        """Load multiple audio files"""
        file_paths = filedialog.askopenfilenames(defaultextension=".mp3", filetypes=[("Audio Files", "*.mp3;*.wav")])
        if file_paths:
            self.playlist = list(file_paths)
            self.song_index = 0
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)

    def toggle_play_pause(self):
        """Toggle between play, pause, and resume"""
        if not self.current_song and self.playlist:
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)

        if pygame.mixer.music.get_busy():
            pygame.mixer.music.pause()
            self.paused = True
            self.play_button.config(text="Resume")
        elif self.paused:
            pygame.mixer.music.unpause()
            self.paused = False
            self.play_button.config(text="Pause")
        else:
            pygame.mixer.music.play()
            self.paused = False
            self.play_button.config(text="Pause")

    def stop_music(self):
        """Stop music playback"""
        pygame.mixer.music.stop()
        self.paused = False
        self.play_button.config(text="Play")

    def next_song(self):
        """Play the next song in the playlist"""
        if self.playlist and self.song_index < len(self.playlist) - 1:
            self.song_index += 1
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)
            pygame.mixer.music.play()
            self.paused = False
            self.play_button.config(text="Pause")

    def previous_song(self):
        """Play the previous song in the playlist"""
        if self.playlist and self.song_index > 0:
            self.song_index -= 1
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)
            pygame.mixer.music.play()
            self.paused = False
            self.play_button.config(text="Pause")

def prev_cb(payload):
    player.previous_song()
    
def next_cb(payload):
    player.next_song()
    
def play_cb(payload):
    player.toggle_play_pause()
    
def pause_cb(payload):
    player.toggle_play_pause()
    
def main():
    global player
    # Create the event handler
    event_handler = EventHandler(ADAFRUIT_BLE_MAC_ADDR, MAX_PACKET_QUEUE_SIZE)
    
    root = tk.Tk()
    player = MusicPlayer(root)
    
    # Setup event
    event_handler.on_event(Events.SONG_SKIP_PREV, prev_cb)
    event_handler.on_event(Events.SONG_SKIP_NEXT, next_cb)
    event_handler.on_event(Events.SONG_PLAY, play_cb)
    event_handler.on_event(Events.SONG_PAUSE, pause_cb)
    root.mainloop()
    
main()