
/* Winamp 3 Player core api v0.1
** (c)2000 nullsoft jcf/ct/dk
** Notes:
**			Keep in mind that this header file is subject to change prior to the
**		release of Winamp 3.  The ability to configure plug-ins has yet to be
**		added and is the first and foremost concern of the engineering team.
*/

#ifndef __CORE_API_H
#define __CORE_API_H

// Visual C 6 makes big unaligned dlls. the following will correct it
#ifndef _DEBUG
// release optimizations
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
#pragma optimize("gsy",on)
#pragma comment(linker,"/RELEASE")
// set the 512-byte alignment
#pragma comment(linker,"/opt:nowin98")
#endif

// Use Assert in your code to catch errors that shouldn't happen, when compiled in release mode, they are #defined out
#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(x) if (!(x)) MessageBox(NULL,"ASSERT FAILED: " #x,"ASSERT FAILED in " __FILE__ ,MB_OK|MB_ICONSTOP);
#else
#define ASSERT(x)
#endif
#endif



/* CLASS DESCRIPTIONS */

/* WReader
** File reader module class, ie. opens and reads files, streams
*/
class WReader;

/* WInputInfo
** Class that returns information (length, title, metadata) about a specified file
*/
class WInputInfo;

/* WInfo_callback
** Player's interface that provides Winamp 3 core functions to your WInputInfo classes
*/
class WInfo_callback;

/* WInputSource
** Input Source manager base class, ie. decodes mp3's, wavs
*/
class WInputSource;

/* WOutputFilter
** Abstract base class for any Output Filter plug-in, ie. changes pitch, converts format, outputs to speakers
*/
class WOutputFilter;

/* WPlayer_callback
** Player's interface that provides Winamp 3 core functions to your Input Sources and Output Filter plug-ins
** (Getting a reader for opening a file, sending stuff about what's going on to the Winamp 3 core)
*/
class WPlayer_callback;




class WPlayer_callback
{
  public:
	/* GetReader
	** Allows your Input Source and Output Filter plugins to request a reader from Winamp,
	** so you don't have to worry about opening files or streams
	*/
    virtual WReader *GetReader(char *url)=0;


	/* The 3 following functions allows your Input Source and Output Filter plugins to send error/warning/status
	** messages back to the Winamp 3 core
	*/

	/* Error
	** playback should stop (soundcard driver error, etc)
	*/
	virtual void Error(char *reason)=0;

	/* Warning
	** warning (something recoverable, like file not found, etc)
	*/
	virtual void Warning(char *warning)=0;

	/* Status
	** status update (nothing really wrong)
	*/
	virtual void Status(char *status)=0;




	/* TitleChange
	** should be called if the current file titlename changes during the decoding
	*/
	virtual void TitleChange(char *new_title)=0;

	/* InfoChange
	** should be called if the infos about the current file changes during the decoding
	*/
	virtual void InfoChange(char *new_info_str, int new_length)=0;

	/* UrlChange
	** should be called if the current file associated URL changes during the decoding
	*/
	virtual void UrlChange(char *new_url)=0;
};




class WInfo_callback
{
  public:
	/* GetReader
	** Allows your WInfo classes to request a reader from Winamp
	** so you don't have to worry about opening files or streams
	*/
    virtual WReader *GetReader(char *url)=0;
};




class WInputInfo
{
  public:
	/* WInputInfo
	** WInputInfo constructor
	*/
	WInputInfo(){ };

	/* m_info
	** Filled by Winamp. Pointer to WInputInfo callback function
	*/
	WInfo_callback *m_info;

	/* Open
	** Called by Winamp to request informations about a specified media (file, url, etc...)
	** returns 0 if succesful, 1 if not
	**
	** You must open, get all information and close the specified file here and store
	** the useful information into member elements for quick access by other functions
	*/
	virtual int  Open(char *url) { return 1; }

	/* GetTitle
	** Called by Winamp to get the decoded title about the file opened
	** i.e. id3 title name, etc...
	*/
	virtual void GetTitle(char *buf, int maxlen) { if (maxlen>0) buf[0]=0; };

	/* GetInfoString
	** Called by Winamp to get extra informations about the file opened
	** i.e. "160kbps stereo 44Khz" for MP3 files,"4 channels" for MOD files,etc...
	*/
	virtual void GetInfoString(char *buf, int maxlen) { if (maxlen>0) buf[0]=0; };

	/* GetLength
	** Called by Winamp to retrieves media type length in milliseconds
	** returns -1 if length is undefined/infinite
	*/
	virtual int  GetLength(void) { return -1; };

	/* GetMetaData
	** Fetches metadata by attribute name (Artist, Album, Bitrate, etc...)
	** attribute names are non case-sensitive.
	** returns size of data
	*/
	virtual int  GetMetaData(char *name, char *data, int data_len) { if (data&&data_len>0) *data=0; return 0; }

	/* ~WInputInfo
	** WInputInfo virtual destructor
	*/
	virtual ~WInputInfo() { };
};








/* WINAMP Output Filter NOTIFY MESSAGES
**	Messages returned to notify Output Filter plug-ins of events
*/

typedef enum {

	/* WOFNM_FILETITLECHANGE
	** Sent when the song changes
	** param1=new filename song
	** param2=new title song
	*/
	WOFNM_FILETITLECHANGE=1024,

	/* WOFNM_ENDOFDECODE
	** Sent when decoding ends
	*/
	WOFNM_ENDOFDECODE,

} WOutputFilterNotifyMsg;




class WOutputFilter
{
  protected:
	/* WOutputFilter
	** WOutputFilter constructor
	*/
	WOutputFilter() { m_next=NULL; };

  public:
	/* m_player
	** Filled by Winamp. Pointer to Winamp 3 core player interface
	*/
	WPlayer_callback *m_player;

	/* m_next
	** Internally used by Winamp. Pointer to next activated Output Filter
	*/
	WOutputFilter *m_next;

	/* ~WOutputFilter
	** WOutputFilter destructor
	*/
	virtual ~WOutputFilter() { };

	/* GetDescription
	** Retrieves your plug-in's text description
	*/
	virtual char *GetDescription() { return "Unknown"; };

	/* ProcessSamples
	** Render data as it receives it
	** sampledata: Data to process
	** bytes: number of bytes to process
	** bps: Bits per sample (8 or 16)
	** nch: Number of channels (1 or 2)
	** srate: Sample rate in Hz
	** killswitch: Will be set to 1 by winamp if stop if requested. Poll the pointed value very often to
	**             make sure Winamp doesn't hang
	**
	** Returns the number of processed bytes or -1 if unable to open the device or an error occured.
	**
	** You have to open your device (ie. Directsound) the first time this function is called.
	*/
	virtual int ProcessSamples(char *sampledata, int bytes, int *bps, int *nch, int *srate, bool *killswitch) { return bytes; }

	/* FlushSamples
	** Flushes output buffers so that all is written
	*/
	virtual void FlushSamples(bool *killswitch) { };

	/* Restart
	** Called by Winamp after a seek
	*/
	virtual void Restart(void) { }

	/* GetLatency
	** Returns < 0 for a final output latency, > 0 for an additive
	*/
	virtual int  GetLatency(void) { return 0; }

	/* Pause
	** Suspends output
	*/
	virtual void Pause(int pause) { }

	/* ShutDown
	** Completely stops output
	**
	** Close your device here (not in destructor)
	*/
	virtual void ShutDown(void) { }

	/* SetVolume
	** Sets the volume (0 to 255)
	** return 1 if volume successfully modified
	*/
	virtual int SetVolume(int volume) { return 0; }

	/* SetPan
	** Sets Left-Right sound balance (-127 to 127)
	** return 1 if pan successfully modified
	*/
	virtual int SetPan(int pan) { return 0; }

	/* Notify
	** Called by Winamp to notify what's going on
	*/
	virtual void Notify(WOutputFilterNotifyMsg msg, int data1, int data2) { }

};




class WInputSource
{

  protected:
	/* WInputSource
	** WInputSource constructor
	*/
  WInputSource(){ };


  public:
	/* m_player
	** Filled by Winamp. Pointer to Winamp 3 core interface
	*/
	WPlayer_callback *m_player;

	/* GetDescription
	** Retrieves your plug-in's text description
	*/
	virtual char *GetDescription() { return "Unknown"; };

	/* UsesOutputFilters
	** Returns whether or not the Output Filter pipeline can be used
	*/
	virtual int UsesOutputFilters(void) { return 1; }

	/* Open
	** Used to open and prepare input media type
	*/
	virtual int Open(char *url, bool *killswitch)=0;

	/* GetSamples
	** This function must fill bps, nch and srate.
	** Here, you have to fill the sample_buffer with decoded data. Be sure to fill it with the specified
	** size (bytes). Use an internal buffer, etc ...
	**
	** sample_buffer: buffer to put decoded data into
	** bytes: size of the sample_buffer
	** bps: Bits par sample (8 or 16)
	** nch: Number of channels (1 or 2)
	** srate: Sample rate in Hz
	** killswitch: Will be set to 1 by winamp if stop if requested. Poll the pointed value very often to
	**              make sure Winamp doesn't hang
	*/
	virtual int GetSamples(char *sample_buffer, int bytes, int *bps, int *nch, int *srate, bool *killswitch)=0;

	/* SetVolume
	** Sets the volume (0 to 255)
	** Return 1 if volume has been set
	*/
	virtual int SetVolume(int volume) { return 0; };

	/* SetPan
	** Sets Left-Right sound balance (-127 to 127)
	** return 1 if pan successfully modified
	*/
	virtual int SetPan(int pan) { return 0; };

	/* SetPosition
	** Sets position in ms. returns 0 on success, 1 if seek impossible
	*/
	virtual int SetPosition(int)=0;

	/* Pause
	** Suspends input
	*/
	virtual void Pause(int pause) { };

	/* GetPosition
	** Retrieve position in milliseconds
	*/
	virtual int GetPosition(void) { return 0; }

	/* GetTitle
	** Called by Winamp to get the decoded title about the file opened
	** i.e. stream name, id3 title name, etc...
	*/
	virtual void GetTitle(char *buf, int maxlen) { if(maxlen>0) buf[0]=0; };

	/* GetInfoString
	** Called by Winamp to get extra informations about the file openend
	** i.e. "32kbps 44khz", etc...
	*/
	virtual void GetInfoString(char *buf, int maxlen) { if(maxlen>0) buf[0]=0; };

	/* GetLength
	** Called by Winamp to retrieves media type length in milliseconds
	** returns -1 if length is undefined/infinite
	*/
	virtual int GetLength(void) { return -1; }

	/* ~WInputSource
	** ~WInputSource virtual destructor
	*/
	virtual ~WInputSource() { };
};




class WReader
{
  protected:

	/* WReader
	** WReader constructor
	*/
	WReader() { }

  public:

	/* m_player
	** Filled by Winamp. Pointer to Winamp 3 core interface
	*/
	WPlayer_callback *m_player;

	/* GetDescription
	** Retrieves your plug-in's text description
	*/
	virtual char *GetDescription() { return "Unknown"; };

	/* Open
	** Used to open a file, return 0 on success
	*/
	virtual int Open(char *url, bool *killswitch)=0;

	/* Read
	** Returns number of BYTES read (if < length then eof or killswitch)
	*/
	virtual int Read(char *buffer, int length, bool *killswitch)=0;

	/* GetLength
	** Returns length of the entire file in BYTES, return -1 on unknown/infinite (as for a stream)
	*/
	virtual int GetLength(void)=0;

	/* CanSeek
	** Returns 1 if you can skip ahead in the file, 0 if not
	*/
	virtual int CanSeek(void)=0;

	/* Seek
	** Jump to a certain absolute position
	*/
	virtual int Seek(int position, bool *killswitch)=0;

	/* GetHeader
	** Retrieve header. Used in read_http to retrieve the HTTP header
	*/
	virtual char *GetHeader(char *name) { return 0; }

	/* ~WReader
	** WReader virtual destructor
	*/
	virtual ~WReader() { }
};




/* DLL PLUGINS EXPORT STRUCTURES */

#define READ_VER	0x100
#define IN_VER		0x100
#define OF_VER		0x100


typedef struct
{
	/* version
	** Version revision number
	*/
	int version;

	/* description
	** Text description of the reader plug-in
	*/
	char *description;

	/* create
	** Function pointer to create a reader module
	*/
	WReader *(*create)();

	/* ismine
	** Determines whether or not a file should be read by this plug-in
	*/
	int (*ismine)(char *url);

} reader_source;




typedef struct
{
	/* version
	** Version revision number
	*/
	int version;

	/* description
	** Text description of the input plug-in
	*/
	char *description;

	/* extension_list
	** Defines all the supported filetypes by this input plug-in
	** In semicolon delimited format ("ext;desc;ext;desc" etc).
	*/
	char *extension_list;

	/* ismine
	** called before extension checks, to allow detection of tone://,http://, etc
	** Determines whether or not a file type should be decoded by this plug-in
	*/
	int (*ismine)(char *filename);

	/* create
	** Function pointer to create a decoder module
	*/
	WInputSource *(*create)(void);

	/* createinfo
	** Function pointer to create a decoder module information
	*/
	WInputInfo *(*createinfo)(void);

} input_source;



typedef struct
{
	/* version
	** Version revision number
	*/
	int version;

	/* description
	** Text description of the output plug-in
	*/
	char *description;

	/* create
	** Function pointer to create an Output Filter
	*/
	WOutputFilter *(*create)();

} output_filter;


#endif
