import tkinter as tk
from tkinter import Listbox, Scrollbar
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
        self.music_folder = "musicfile"  # Default folder

        # Initialize pygame mixer
        pygame.mixer.init()

        # Listbox to display song list
        self.listbox = Listbox(master, width=50, height=15)
        self.listbox.pack()
        self.listbox.bind("<<ListboxSelect>>", self.select_song)

        # Scrollbar
        self.scrollbar = Scrollbar(master)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.listbox.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.listbox.yview)

        # Buttons
        self.play_button = tk.Button(master, text="Play", command=self.toggle_play_pause)
        self.play_button.pack()

        self.stop_button = tk.Button(master, text="Stop", command=self.stop_music)
        self.stop_button.pack()

        self.next_button = tk.Button(master, text="Next", command=self.next_song)
        self.next_button.pack()

        self.previous_button = tk.Button(master, text="Previous", command=self.previous_song)
        self.previous_button.pack()

        # Auto-load songs from the musicfile directory
        self.load_music_folder()

    def load_music_folder(self):
        """Automatically loads all music files from 'musicfile' folder"""
        if os.path.exists(self.music_folder):
            self.playlist = [os.path.join(self.music_folder, f) for f in os.listdir(self.music_folder) if f.endswith((".mp3", ".wav"))]
            self.listbox.delete(0, tk.END)  # Clear existing items
            for song in self.playlist:
                self.listbox.insert(tk.END, os.path.basename(song))  # Display only file names
            
            if self.playlist:
                self.song_index = 0
                self.current_song = self.playlist[self.song_index]
                pygame.mixer.music.load(self.current_song)

    def select_song(self):
        """Play selected song from the listbox"""
        selected_index = self.listbox.curselection()
        if selected_index:
            self.song_index = selected_index[0]
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)
            pygame.mixer.music.play()
            self.paused = False
            self.play_button.config(text="Pause")

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
        if self.playlist:
            self.song_index = (self.song_index + 1) % (len(self.playlist))
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)
            pygame.mixer.music.play()
            self.update_listbox_selection()
            self.pause = False
            self.play_button.config(text="Pause")
           

    def previous_song(self):
        """Play the previous song in the playlist"""
        if self.playlist:
            self.song_index = (self.song_index - 1) % (len(self.playlist))
            self.current_song = self.playlist[self.song_index]
            pygame.mixer.music.load(self.current_song)
            pygame.mixer.music.play()
            self.update_listbox_selection()
            self.pause = False
            self.play_button.config(text="Pause")

    def update_listbox_selection(self):
            """Update the listbox selection to reflect the currently playing song"""
            self.listbox.selection_clear(0, tk.END)
            self.listbox.selection_set(self.song_index)
            self.listbox.activate(self.song_index)

    def select_up(self):
        """Scroll upward (moves selection downward) with a 500ms delay before updating the selection."""
        if self.playlist:
            self.song_index = (self.song_index + 1) % len(self.playlist)
            self.update_listbox_selection()

    def select_down(self):
        """Scroll downward (moves selection upward) with a 500ms delay before updating the selection."""
        if self.playlist:
            self.song_index = (self.song_index - 1) % len(self.playlist)
            self.update_listbox_selection()



def prev_cb(payload):
    player.previous_song()
    
def next_cb(payload):
    player.next_song()
    
def play_cb(payload):
    player.toggle_play_pause()
    
def pause_cb(payload):
    player.toggle_play_pause()

def selectUp_cb(payload):
    print("select up")
    player.select_up()

def selectDown_cb(payload):
    print("select down")
    player.select_down()

def selectMusic_cb(payload):
    player.select_song()
    
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
    event_handler.on_event(Events.MUSIC_SELECT_UP, selectUp_cb)
    event_handler.on_event(Events.MUSIC_SELECT_DOWN, selectDown_cb)
    event_handler.on_event(Events.MUSIC_SELECT, selectMusic_cb)
    root.mainloop()
    
main()