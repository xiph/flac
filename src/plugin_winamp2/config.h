
//
//  common stuff
//

typedef struct {
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
} output_config_t;

typedef struct {
	struct {
		char tag_format[256];
	} title;
	output_config_t output;
} flac_config_t;

extern flac_config_t flac_cfg;
extern char ini_name[MAX_PATH];

//
//  prototypes
//

void ReadConfig();
void WriteConfig();
int DoConfig(HINSTANCE inst, HWND parent);
