#ifndef _ICC_Decoder_H_
#define _ICC_Decoder_H_
 
 class ICC_Decoder
 {
	 public:
		 ICC_Decoder(){};
		virtual ~ICC_Decoder(){};
		virtual int ParsePacket(char * data, int len)=0;
 };

 
#endif


