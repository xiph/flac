
//
//  common stuff
//

typedef struct {
    //!
    /*
	struct {
		gboolean tag_override;
		gchar *tag_format;
		gboolean convert_char_set;
		gchar *file_char_set;
		gchar *user_char_set;
	} title;
    */

	struct {
		struct {
			BOOL enable;
			BOOL album_mode;
			INT  preamp;
			BOOL hard_limit;
		} replaygain;
		struct {
			struct {
				BOOL dither_24_to_16;
			} normal;
			struct {
				BOOL dither;
				INT  noise_shaping; /* value must be one of NoiseShaping enum, c.f. plugin_common/replaygain_synthesis.h */
				INT  bps_out;
			} replaygain;
		} resolution;
	} output;
} flac_config_t;

extern flac_config_t flac_cfg;
extern char ini_name[MAX_PATH];
extern HANDLE thread_handle;
extern In_Module mod_;

//
//  prototypes
//

void ReadConfig();
void WriteConfig();
int DoConfig(HWND parent);
