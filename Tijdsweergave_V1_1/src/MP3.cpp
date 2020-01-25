#include "MP3.h"
#include <Arduino.h>

// Empty constructor
MP3::MP3(){
  this->serial = &Serial;
}

// Private functions
int MP3::sendToModule(dtCommand udtCommand)
{
  unsigned char ucData[10];
  unsigned int uiCRC;

  // Assign correct data to buffer
  for(int i = 0 ; i < 10; i++)
  {
    switch(i)
    {
      case index_mp3_command:
        ucData[i] = udtCommand.ucCommand;
        break;
        
      case index_mp3_paramHigh:
        ucData[i] = (udtCommandData.uiParameter >> 8) & 0xFF;
        break;
        
      case index_mp3_paramLow:
        ucData[i] = udtCommandData.uiParameter & 0xFF;
        break;
        
      default:
        ucData[i] = ucDataMask[i];
        break;
    }
  }
  
  uiCRC = calculateCRC(ucData);
  ucData[index_mp3_CRCHigh] = (uiCRC >> 8) & 0xFF;
  ucData[index_mp3_CRCLow] = uiCRC & 0xFF;

  return serial->write(ucData, 10);
}

unsigned int MP3::calculateCRC(unsigned char *data)
{
  unsigned int uiSum;

  uiSum = 0;
  for(int i = 1; i <= 6; i++)
  {
    uiSum += *(data + i);
  }
  return -uiSum;
}

int MP3::receiveFromModule()
{
  int iReturnState;
  static unsigned char ucBuffer[10];
  static unsigned int uiBufferIndex = 0;
  static unsigned long ulTimer;
  unsigned char ucCurrentChar;
  unsigned int uiCRC, uiReceivedCRC;

  iReturnState = -1;
  
  if(serial->available() > 0 )
  {
    ucCurrentChar = serial->read();
    if(uiBufferIndex < MP3_UART_BUFFER_SIZE)
    {
      ucBuffer[uiBufferIndex] = ucCurrentChar;
      uiBufferIndex++;
    }
    ulTimer = millis() + MP3_CHARACTER_TIMEOUT;
  }
  
  if(millis() >= ulTimer && uiBufferIndex != 0)
  {
    
    if(uiBufferIndex >= 10)
    {
      uiReceivedCRC = (ucBuffer[index_mp3_CRCHigh] << 8) | ucBuffer[index_mp3_CRCLow];
      uiCRC = calculateCRC(ucBuffer);
      
      if(uiCRC == uiReceivedCRC)
      {
        iReturnState = ucBuffer[index_mp3_command];
      }
      // else ignore the data.
    }
    // Empty buffer to receive next frame
    uiBufferIndex = 0;
  }
  return iReturnState;
}

// Public functions
void MP3::begin()
{
  serial->begin(9600);
}

// Main function
int MP3::handle()
{
  static unsigned long ulTimer = 0;

  int iModuleState;
  int iReturnState;

  iModuleState = receiveFromModule();

  iReturnState = 0;
  
  switch(iStep)
  {
    case 0:
      switch(iModuleState)
      {
        case state_mp3_donePlayingTrackOnUSB:
        case state_mp3_donePlayingTrackOnSD:
        //case state_mp3_NA3:
        case state_mp3_initialisation:
          udtModuleState.xIsPlaying = false;
          udtModuleState.xIsPaused = false;
		  udtModuleState.xIsRepeating = false;
          break;
          
        case state_mp3_error:
        //case state_mp3_feedback:
        case state_mp3_status:
        case state_mp3_volume:
        case state_mp3_EQ:
        //case state_mp3_NA4:
        //case state_mp3_NA5:
        case state_mp3_amountOfFilesOnUSB:
        case state_mp3_amountOfFilesOnSD:
        //case state_mp3_NA6:
        //case state_mp3_NA7:
        case state_mp3_currentTrackOnUSB:
        case state_mp3_currentTrackOnSD:
        //case state_mp3_NA8:
        case state_mp3_amountOfFilesInFolder:
        case state_mp3_amountOfFolders:
        default: 
          break;
      }
      break;
      
    case 1:
      // Got command from a function
      sendToModule(udtCommandData);
      ulTimer = millis() + MP3_FEEDBACK_TIMEOUT;
      iStep++;
      break;

    case 2:
      // Feedback from module
      switch(iModuleState)
      {
        case -1:
          if(millis() >= ulTimer)
          {
            iStep = 900;
          }
          break;
        case state_mp3_feedback:
          iStep++;
          break;
        case state_mp3_error:
          iStep = 901;
          break;
        default:
          iStep = 902;
          break;
      }
      break;

    case 3: // Update module state (if needed)
            // Sent data to module, has to be a command      
      switch(udtCommandData.ucCommand)
      {
        // Set playback state
		case command_mp3_repeatFolder:
			udtModuleState.xIsPlaying = true;
			udtModuleState.xIsPaused = false;
			udtModuleState.xIsRepeating = true;
			udtModuleState.xIsPlayingBackToBack = true;
			break;
		
		// Set playback state
        case command_mp3_next:
        case command_mp3_previous:
        case command_mp3_playTrackFromRoot:
        case command_mp3_repeatTrackFromRoot:
        case command_mp3_playTrackFromFolder:
        case command_mp3_repeatAll:
        case command_mp3_playTrackFromMP3Folder:
        case command_mp3_random:
        //case command_mp3_repeatFolder:
        case command_mp3_playTrackFromLargeFolder:
		case command_mp3_play:
			udtModuleState.xIsPlaying = true;
			udtModuleState.xIsPaused = false;
			udtModuleState.xIsRepeating = false;
			udtModuleState.xIsPlayingBackToBack = false;
          break;
		  
		case command_mp3_stop:
		case command_mp3_reset:
			udtModuleState.xIsPlaying = false;
			udtModuleState.xIsPaused = false;
			udtModuleState.xIsRepeating = false;
			udtModuleState.xIsPlayingBackToBack = false;
			break;
		
		case command_mp3_pause:
			udtModuleState.xIsPlaying = false;
			udtModuleState.xIsPaused = true;
			//udtModuleState.xIsRepeating = false;
			//udtModuleState.xIsPlayingBackToBack = false;
			break;
			
		//case command_mp3_increaseVolume:
		//case command_mp3_decreaseVolume:
		//case command_mp3_specifyVolume:
		//case command_mp3_specifyEQ:
		//case command_mp3_specifyPlaybackDevice:
		//case command_mp3_enterStandby:
		//case command_mp3_reserved:
		//case command_mp3_setAmplification:
		//case command_mp3_intercutAdvertisement:
		//case command_mp3_stopAdvertisement:
		//case command_mp3_repeatCurrentTrack:
		//case command_mp3_DacToggle:
		default:
		  break;
		}
		iStep = 0;
		break;

	// Errors
	case 900: // No feedback from module
	case 901: // Got error from module
	case 902: // Unexpected feedback from module
		iReturnState = -1;
		iStep = 0;
		udtModuleState.xIsPlaying = false;
		udtModuleState.xIsPaused = false;
		udtModuleState.xIsRepeating = false;
		
		break;

	default:  
		iStep = 0;
		break;
	}
	return iReturnState;
}

// Play sound from folder
int MP3::playSound(int iFolder, int iTrack)
{
  int iReturnState = 0;
  
  if(iFolder > 99) iFolder = 99;
  if(iFolder < 1) iFolder = 1;
  if(iTrack > 255) iTrack = 255;
  if(iTrack < 1) iTrack = 1;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_playTrackFromFolder;
    udtCommandData.uiParameter = (iFolder << 8) + iTrack;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}

// Play sound from MP3 folder
int MP3::playMP3(int iTrack)
{
  int iReturnState = 0;
  
  if(iTrack > 2999) iTrack = 2999;
  if(iTrack < 1) iTrack = 1;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_playTrackFromMP3Folder;
    udtCommandData.uiParameter = iTrack;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}

int MP3::repeatFolder(int iFolder)
{
  int iReturnState = 0;
  
  if(iFolder > 99) iFolder = 99;
  if(iFolder < 1) iFolder = 1;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_repeatFolder;
    udtCommandData.uiParameter = iFolder;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}

// Stop playback
int MP3::stop()
{
  int iReturnState = 0;
    
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_stop;
    udtCommandData.uiParameter = 0;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}

// Pause playback
int MP3::pause()
{
  int iReturnState = 0;
    
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_pause;
    udtCommandData.uiParameter = 0;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}
int MP3::play()
{
  int iReturnState = 0;
    
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_play;
    udtCommandData.uiParameter = 0;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}

// Repeat current track
int MP3::setRepeat(int iRepeatState)
{
  int iReturnState = 0;
    
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_repeatCurrentTrack;
    udtCommandData.uiParameter = (iRepeatState != 0 ? 0 : 1);
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}
int MP3::repeat()
{
  setRepeat(1);
}
int MP3::noRepeat()
{
  setRepeat(0);
}

// Set module volume
int MP3::setVolume(int iVolumeLevel)
{
  int iReturnState = 0;
  
  if(iVolumeLevel > 30) iVolumeLevel = 30;
  if(iVolumeLevel < 1) iVolumeLevel = 1;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_specifyVolume;
    udtCommandData.uiParameter = iVolumeLevel;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}
int MP3::increaseVolume()
{
  int iReturnState = 0;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_increaseVolume;
    udtCommandData.uiParameter = 0;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}
int MP3::decreaseVolume()
{
  int iReturnState = 0;
  
  if(iStep == 0)
  {
    iStep = 1;
    udtCommandData.ucCommand = command_mp3_decreaseVolume;
    udtCommandData.uiParameter = 0;
  }
  else
    iReturnState = -1;
    
  return iReturnState;
}
int MP3::getPlayState()
{
  return udtModuleState.xIsPlaying;
}
