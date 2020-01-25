#ifndef MP3_H
#define MP3_H

#include <Arduino.h>

#define MP3_FEEDBACK_TIMEOUT 3000 // Error if no feedback has been seen in 3 seconds from the moment data has been sent to the MP3 module.
#define MP3_CHARACTER_TIMEOUT 30  // 30ms should be about right to receive a single character from the MP3 module.
#define MP3_UART_BUFFER_SIZE 10   // Buffer size. Greater buffer size fills up RAM pretty easily. You never need more than 10 bytes.

class MP3
{
private:
  // Private enumerated types
  enum commands
  {
    command_mp3_next = 0x01,
    command_mp3_previous = 0x02,
    command_mp3_playTrackFromRoot = 0x03,
    command_mp3_increaseVolume = 0x04,
    command_mp3_decreaseVolume = 0x05,
    command_mp3_specifyVolume = 0x06,
    command_mp3_specifyEQ = 0x07,
    command_mp3_repeatTrackFromRoot = 0x08,
    command_mp3_specifyPlaybackDevice = 0x09,
    command_mp3_enterStandby = 0x0A,
    command_mp3_reserved = 0x0B,
    command_mp3_reset = 0x0C,
    command_mp3_play = 0x0D,
    command_mp3_pause = 0x0E,
    command_mp3_playTrackFromFolder = 0x0F,
    command_mp3_setAmplification = 0x10,
    command_mp3_repeatAll = 0x11,
    command_mp3_playTrackFromMP3Folder = 0x12,
    command_mp3_intercutAdvertisement = 0x13,
    command_mp3_playTrackFromLargeFolder = 0x14,
    command_mp3_stopAdvertisement = 0x15,
    command_mp3_stop = 0x16,
    command_mp3_repeatFolder = 0x17,
    command_mp3_random = 0x18,
    command_mp3_repeatCurrentTrack = 0x19,
    command_mp3_DacToggle = 0x1A,
  };
  enum states
  {
    state_mp3_donePlayingTrackOnUSB = 0x3C, // Done playing track on USB
    state_mp3_donePlayingTrackOnSD = 0x3D, // Done playing track on SD
    state_mp3_NA3 = 0x3E,
    state_mp3_initialisation = 0x3F,
    state_mp3_error = 0x40,
    state_mp3_feedback = 0x41,
    state_mp3_status = 0x42,
    state_mp3_volume = 0x43,
    state_mp3_EQ = 0x44,
    state_mp3_NA4 = 0x45,
    state_mp3_NA5 = 0x46,
    state_mp3_amountOfFilesOnUSB = 0x47,
    state_mp3_amountOfFilesOnSD = 0x48,
    state_mp3_NA6 = 0x49,
    state_mp3_NA7 = 0x4A,
    state_mp3_currentTrackOnUSB = 0x4B,
    state_mp3_currentTrackOnSD = 0x4C,
    state_mp3_NA8 = 0x4D,
    state_mp3_amountOfFilesInFolder = 0x4E,
    state_mp3_amountOfFolders = 0x4F,
  };
  enum dataIndex
  {
    index_mp3_command = 3,
    index_mp3_paramHigh = 5,
    index_mp3_paramLow = 6,
    index_mp3_CRCHigh = 7,
    index_mp3_CRCLow = 8,
  };
  
  // Private globals
  int iStep = 0;
  struct dtCommand
  {
    unsigned char ucCommand;
    unsigned int uiParameter;    
  } udtCommandData;
  
  struct dtModuleState
  {
    bool xIsPlaying = false;
    bool xIsPaused = false;
	bool xIsRepeating = false;
	bool xIsPlayingBackToBack = false;
  } udtModuleState;
  
  const unsigned char ucDataMask[10] =
  {
    0x7E, 0xFF, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xEF
  };
  
  // Private functions
  unsigned int calculateCRC(unsigned char *data);
  int sendToModule(dtCommand);
  int receiveFromModule();

  HardwareSerial *serial;

public:
  // Public functions
  
  // Constructor
  MP3();
  
  // Initialisation functions
  void begin();
  
  // Main function
  int handle();
  
  // Play sounds
  int playSound(int iFolder, int iTrack);
  
  // Play sound from MP3 folder
  int playMP3(int iTrack);
  
  // Repeat all files in a folder
  int repeatFolder(int iFolder);
  
  // Stop playback
  int stop();

  // Pause playback
  int pause();

  int play();
  
  // Repeat single file
  int setRepeat(int iRepeatState);
  int repeat();
  int noRepeat();
  
  // Set volume
  int setVolume(int iVolume);
  int increaseVolume();
  int decreaseVolume();

  // Getters
  int getPlayState();
};

#endif
