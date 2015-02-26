//#if 1
#include <stdio.h>
#include <stdlib.h>
#include "stdafx.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "myHead.h"
#ifdef __cplusplus
};
#endif

#include "sdl\SDL.h"
#include "sdl\SDL_thread.h"

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#if 0
//void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
//	FILE *pFile;
//	char szFilename[32];
//	int  y;
//
//	// Open file
//	sprintf(szFilename, "frame%d.ppm", iFrame);
//	pFile=fopen(szFilename, "wb");
//	if(pFile==NULL)
//		return;
//
//	// Write header
//	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
//
//	// Write pixel data
//	for(y=0; y<height; y++)
//		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
//
//	// Close file
//	fclose(pFile);
//}
#endif

#ifdef SET_MFC_WINDOW
int main_ffplay(char* argc, char **argv)
#else
int main_ffplay()
#endif
{
	AVFormatContext *pFormatCtx;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame; 
	AVFrame         *pFrameRGB;
	AVPacket        packet;
	int             frameFinished;
	int             numBytes;
	uint8_t         *buffer;
	float           aspect_ratio;
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;
	AVPicture pict;
	static struct SwsContext *img_convert_ctx;
	char * filePath="rtsp://192.168.1.168:8557/PSIA/Streaming/channels/2?videoCodecType=H.264";
	//"rtsp://192.168.250.217:554/h264";
	//"rtsp://192.168.230.151:554/axis-media/media.amp?videocodec=h264";
	
	av_register_all();// Register all formats and codecs

#ifdef SET_MFC_WINDOW  //defined in "myHead.h"
	char *myvalue;
	SDL_putenv(argc); 
	myvalue = SDL_getenv("SDL_WINDOWID");
#endif
	if(SDL_Init(SDL_INIT_VIDEO /*| SDL_INIT_AUDIO*/ | SDL_INIT_TIMER)) 
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	// Open video file
	if(av_open_input_file(&pFormatCtx, filePath, NULL, 0, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, filePath, 0);

	// Find the first video stream
	videoStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) 
		{
			videoStream=i;
			break;
		}
		if(videoStream==-1)
			return -1; // Didn't find a video stream
		// Get a pointer to the codec context for the video stream
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Open codec
		if(avcodec_open(pCodecCtx, pCodec)<0)
			return -1; // Could not open codec

		// Allocate video frame
		pFrame=avcodec_alloc_frame();

		// Allocate an AVFrame structure
		pFrameRGB=avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			return -1;

		//pCodecCtx->width=100;
		//pCodecCtx->height=100;
		// Make a screen to put our video
#ifndef __DARWIN__
		screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
		screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
		if(!screen) {
			fprintf(stderr, "SDL: could not set video mode - exiting\n");
			exit(1);
		}



		// Allocate a place to put our YUV image on that screen
		bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
			pCodecCtx->height,
			SDL_YV12_OVERLAY,
			screen);


		// Determine required buffer size and allocate buffer
		numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

		// Read frames and save first five frames to disk
		i=0;
		while(av_read_frame(pFormatCtx, &packet)>=0) {
			if(packet.stream_index==videoStream) {
				// Decode video frame
				avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
				if(frameFinished) {


					SDL_LockYUVOverlay(bmp);

					pict.data[0] = bmp->pixels[0];
					pict.data[1] = bmp->pixels[2];
					pict.data[2] = bmp->pixels[1];

					pict.linesize[0] = bmp->pitches[0];
					pict.linesize[1] = bmp->pitches[2];
					pict.linesize[2] = bmp->pitches[1];

					// Convert the image from its native format to RGB
					img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
					// Convert the image from its native format to RGB
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,0, pCodecCtx->height, pict.data, pict.linesize);

					SDL_UnlockYUVOverlay(bmp);

					rect.x = 0;
					rect.y = 0;
					rect.w = pCodecCtx->width;
					rect.h = pCodecCtx->height;
					SDL_DisplayYUVOverlay(bmp, &rect);
				}
			}
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
			SDL_PollEvent(&event);
			switch(event.type) {
			case SDL_QUIT:
				SDL_Quit();
				exit(0);
				break;
			default:
				break;
			}


		}
		// Free the RGB image
		av_free(buffer);
		av_free(pFrameRGB);

		// Free the YUV frame
		av_free(pFrame);

		// Close the codec
		avcodec_close(pCodecCtx);

		// Close the video file
		av_close_input_file(pFormatCtx);

		printf("执行完毕\n");
		system("pause");
		return 0;
}
//#else
//#include <stdio.h>
//#include <stdlib.h>
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//#ifdef __cplusplus
//}
//#endif
//
//#include "sdl\SDL.h"
//#include "sdl\SDL_thread.h"
//
//#ifdef __MINGW32__
//#undef main /* Prevents SDL from overriding main() */
//#endif
//
//#define SDL_AUDIO_BUFFER_SIZE 2048
//
//typedef struct PacketQueue {
//	AVPacketList *first_pkt, *last_pkt;
//	int nb_packets;
//	int size;
//	SDL_mutex *mutex;
//	SDL_cond *cond;
//} PacketQueue;
//
//PacketQueue audioq;
//
//int quit = 0;
//
//void packet_queue_init(PacketQueue *q) {
//	memset(q, 0, sizeof(PacketQueue));
//	q->mutex = SDL_CreateMutex();
//	q->cond = SDL_CreateCond();
//}
//int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
//	AVPacketList *pkt1;
//	if(av_dup_packet(pkt) < 0) {
//		return -1;
//	}
//	pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));//reinterpret_cast强制转换
//	if (!pkt1)
//		return -1;
//	pkt1->pkt = *pkt;
//	pkt1->next = NULL;
//
//	SDL_LockMutex(q->mutex);
//
//
//	if (!q->last_pkt)
//		q->first_pkt = pkt1;
//	else
//		q->last_pkt->next = pkt1;
//	q->last_pkt = pkt1;
//	q->nb_packets++;
//	q->size += pkt1->pkt.size;
//	SDL_CondSignal(q->cond);
//	SDL_UnlockMutex(q->mutex);
//	return 0;
//}
//
//static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
//{
//	AVPacketList *pkt1;
//	int ret;
//
//	SDL_LockMutex(q->mutex);
//
//	for(;;) {
//
//		if(quit) {
//			ret = -1;
//			break;
//		}
//
//		pkt1 = q->first_pkt;
//		if (pkt1) {
//			q->first_pkt = pkt1->next;
//			if (!q->first_pkt)
//				q->last_pkt = NULL;
//			q->nb_packets--;
//			q->size -= pkt1->pkt.size;
//			*pkt = pkt1->pkt;
//			av_free(pkt1);
//			ret = 1;
//			break;
//		} else if (!block) {
//			ret = 0;
//			break;
//		} else {
//			SDL_CondWait(q->cond, q->mutex);
//		}
//	}
//	SDL_UnlockMutex(q->mutex);
//	return ret;
//}
//
//int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {
//
//	static AVPacket pkt;
//	static uint8_t *audio_pkt_data = NULL;
//	static int audio_pkt_size = 0;
//
//	int len1, data_size;
//
//	for(;;) {
//		while(audio_pkt_size > 0) {
//			data_size = buf_size;
//			len1 = avcodec_decode_audio2(aCodecCtx, (int16_t *)audio_buf, &data_size, 
//				audio_pkt_data, audio_pkt_size);
//			if(len1 < 0) {
//				/* if error, skip frame */
//				audio_pkt_size = 0;
//				break;
//			}
//			audio_pkt_data += len1;
//			audio_pkt_size -= len1;
//			if(data_size <= 0) {
//				/* No data yet, get more frames */
//				continue;
//			}
//			/* We have data, return it and come back for more later */
//			return data_size;
//		}
//		if(pkt.data)
//			av_free_packet(&pkt);
//
//		if(quit) {
//			return -1;
//		}
//
//		if(packet_queue_get(&audioq, &pkt, 1) < 0) {
//			return -1;
//		}
//		audio_pkt_data = pkt.data;
//		audio_pkt_size = pkt.size;
//	}
//}
//
//void audio_callback(void *userdata, Uint8 *stream, int len) {
//
//	AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
//	int len1, audio_size;
//
//	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
//	static unsigned int audio_buf_size = 0;
//	static unsigned int audio_buf_index = 0;
//
//	while(len > 0) {
//		if(audio_buf_index >= audio_buf_size) {
//			/* We have already sent all our data; get more */
//			audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
//			if(audio_size < 0) {
//				/* If error, output silence */
//				audio_buf_size = 1024; // arbitrary?
//				memset(audio_buf, 0, audio_buf_size);
//			} else {
//				audio_buf_size = audio_size;
//			}
//			audio_buf_index = 0;
//		}
//		len1 = audio_buf_size - audio_buf_index;
//		if(len1 > len)
//			len1 = len;
//		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
//		len -= len1;
//		stream += len1;
//		audio_buf_index += len1;
//	}
//}
//
//int main_ffplay() {
//	AVFormatContext *pFormatCtx;
//	int             i,  audioStream;
//	int videoStream=-1;
//	AVCodec         *pCodec;
//	AVPacket        packet;
//	int             frameFinished;
//	int             numBytes;
//	float           aspect_ratio;
//
//	AVCodecContext  *aCodecCtx;
//	AVCodec         *aCodec;
//
//	SDL_Overlay     *bmp;
//	SDL_Surface     *screen;
//	SDL_Rect        rect;
//	SDL_Event       event;
//	SDL_AudioSpec   wanted_spec, spec;
//
//	static struct SwsContext *img_convert_ctx;
//	char * filePath="C:\\Documents and Settings\\12393\\My Documents\\Visual Studio 2010\\Projects\\myFfplay\\myFfplay\\Debug\\1.avi";
//	// Register all formats and codecs
//	av_register_all();
//
//	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
//		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
//		exit(1);
//	}
//
//	// Open video file
//	if(av_open_input_file(&pFormatCtx, filePath, NULL, 0, NULL)!=0){
//		printf("不能打开\n");
//		return -1; // Couldn't open file
//	}
//
//
//	// Retrieve stream information
//	if(av_find_stream_info(pFormatCtx)<0)
//		return -1; // Couldn't find stream information
//
//	// Dump information about file onto standard error
//	dump_format(pFormatCtx, 0, filePath, 0);
//	// Find the first video stream
//	audioStream=-1;
//	for(i=0; i<pFormatCtx->nb_streams; i++) {
//		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO &&
//			audioStream < 0) {
//				audioStream=i;
//				break;
//		}
//	}
//
//	if(audioStream==-1)
//		return -1;
//	aCodecCtx=pFormatCtx->streams[audioStream]->codec;
//	// Set audio settings from codec info
//	wanted_spec.freq = aCodecCtx->sample_rate;
//	wanted_spec.format = AUDIO_S16SYS;
//	wanted_spec.channels = aCodecCtx->channels;
//	wanted_spec.silence = 0;
//	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//	wanted_spec.callback = audio_callback;
//	wanted_spec.userdata = aCodecCtx;
//
//	if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
//		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
//		return -1;
//	}
//	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
//	if(!aCodec) {
//		fprintf(stderr, "Unsupported codec!\n");
//		return -1;
//	}
//	avcodec_open(aCodecCtx, aCodec);
//
//	// audio_st = pFormatCtx->streams[index]
//	packet_queue_init(&audioq);
//	SDL_PauseAudio(0);
//
//
//
//
//
//	while(av_read_frame(pFormatCtx, &packet)>=0) {
//		if(packet.stream_index==audioStream) {
//			printf("有音频了?\n");
//			packet_queue_put(&audioq, &packet);
//		} else {
//			printf("没有音频了?\n");
//			av_free_packet(&packet);
//		}
//		// Free the packet that was allocated by av_read_frame
//		SDL_PollEvent(&event);
//		switch(event.type) {
//		case SDL_QUIT:
//			quit = 1;
//			SDL_Quit();
//			exit(0);
//			break;
//		default:
//			break;
//		}
//	}
//	//不加下面这一段，不能放完整个音频
//	while (quit == 0){
//		while (SDL_WaitEvent(&event)){
//			if (event.type == SDL_QUIT){
//				quit = 1;
//			}
//			if (event.type == SDL_KEYDOWN){
//				if (event.key.keysym.sym == SDLK_ESCAPE){
//					quit  =1;
//				}
//			}
//		}
//	}
//	SDL_CloseAudio();
//
//	// Close the video file
//	av_close_input_file(pFormatCtx);
//
//	printf("执行完毕\n");
//	system("pause");
//	return 0;
//}
//#endif