


#define VIDEO_PICTURE_SIZE_AUTO  //set the video size 
#define SET_MFC_WINDOW


#ifdef VIDEO_PICTURE_SIZE_AUTO
	int ffplay(char* argc, char **argv,int w,int h);
#else
	int ffplay(char* argc, char **argv);
#endif

#ifdef SET_MFC_WINDOW
	int main_ffplay(char* argc, char **argv);
#else
	int main_ffplay(void);
#endif

	void toggle_pause(void);
	void step_to_next_frame(void);
	void do_exit();