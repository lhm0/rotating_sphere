#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SdFat.h>

class Playlist {
private:
    JsonDocument playlist;              // JSON document of the active playlist
    int currentIndex;                   // Index of the currently playing track

    bool savePlaylist(String playlistPath);                // Save playlist to file

public:
    struct PlaylistEntry {
        const char* file;
        int repetition;
        bool clockOn;
    };

    Playlist();                         // Constructor with SD card reference
    bool updateNow;                     // if this flag is set, 

    char playingNowTitle[64];                // path of title being displayed
    int playingNowRepetitions;
    FsFile filePlayingNow;

    int framesMax;
    int timePerFrame;

    bool openPlaylistTitle(const char* path);        // opens the file "playingNow", saves "filePlayingNow", reads framesMax and timePerFrame
    bool loadNextImage(uint8_t (*lines)[1792][3]);   // load next image of filePlayingNow into lines array
    bool loadPlaylist(String playlistPath);    // loads new playlist. If RS64 is loaded, playlist has just this entry
    PlaylistEntry nextTitle();            // returns path of next title
    bool autoCreate(const char* path);  // Creates auto_generated play list and loads it
};

#endif
