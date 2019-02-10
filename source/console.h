
struct ProfessionalQualityConsole
{
	char history_buffer[8192];
	char input_buffer[256];

	ProfessionalQualityUIPanel panel;
	u32 history_line_count;
	String history;
	String input;

	bool open;
};
