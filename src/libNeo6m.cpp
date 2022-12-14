#include "libNeo6m.h"

int hexChar2IntLUT[] = {
    // ASCII
    // 0  1   2   3   4   5   6   7   7   8   10 11  12  13  14  15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16-31
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 32-47
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, // 48-63
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 64-79
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80-95
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 96-111
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 112-127
};

int decChar2IntLUT[] =
    {
// ASCII
//0  1   2   3   4   5   6   7   7   8   10 11  12  13  14  15
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-15
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16-31
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 32-47
 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, // 48-63
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 64-79
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80-95
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 96-111
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 112-127
};

NEO6M::NEO6M(neo6mSettings theseSettings){
settings = theseSettings;
}

NEO6M::~NEO6M(){
stopMeasurement();
}
void NEO6M::startMeasurement(){
#ifdef DEBUG
    fprintf(stderr,"starting GPS acquisition...\n")
#endif
//spawn the acquisition thread

if (nullptr != daqThread) {
    // already running
#ifdef DEBUG
    fprintf(stderr,"starting GPS acquisition...\n")
#endif
return;
}

//start the polling thread
daqThread = new std::thread(execPollingThread,this);
};

void NEO6M::stopMeasurement(){
isPollingUart = false;
//here we just wait untill the daq thread stops
if (daqThread != nullptr){
    daqThread->join();
    delete daqThread;
    daqThread = nullptr;
    }
close(fd);
};

void NEO6M::pollUartDev() {
  isPollingUart = true;

  uint buffsize = NMEA_MAX_SENTENCE_SIZE * 2;
  char buff[NMEA_MAX_SENTENCE_SIZE *2]; // this way we can fit at least one full message in regardless
                // of where we start
  char data2Send[NMEA_MAX_SENTENCE_SIZE];
  int nbytes;
  parsedNmeaSent parsedSent;

  fd = open(settings.serialDevice.c_str(), O_RDONLY | O_NOCTTY | O_NDELAY);
  if (fd<0){
      perror("Failed to open file, open returned");
  }
  configurePort(fd);
  tcflush(fd, TCIFLUSH); // BF flush the buffered data, we care only for the

  while (isPollingUart) {
    nbytes = read(fd, &buff, buffsize);
    if (nbytes <= 0) {
      perror("Failed to read from port, read() returned");
      // exit(1);
    }

    if (parseNmeaStr(buff, nbytes, parsedSent) < 0) {
      #ifdef DEBUG
      fprintf(stderr,"Failed to parse NMEA Buffer: %.*s\n",nbytes,buff);
      #endif
    }
    else {//if the parse operation was successfull, we can expect the nmeaSentance Properties to be
    // do nothing
    fillMeasStruct(parsedSent);
    }

  } //end of polling
}

int NEO6M::parseNmeaStr(char* thisSent, int  size, parsedNmeaSent& outputSent) // TODO void pollUartDev(); // TODO
{
    //clear
    memset(&outputSent,0,sizeof(parsedNmeaSent));
    //check if the first char is indeed a start char if not then this is an incomplete message and we will return
    if (*thisSent != NMEA_SENTENCE_START_DELIM){
        #ifdef DEBUG
        fprintf(stderr,"Sentance Incomplete: %.*s\n",size,thisSent);
        #endif
        return(-1);
    }

    //the first 6 chars are always start char and message type
    //the last
    char* checksum;
    checksum = strchr(thisSent,NMEA_SENTENCE_CHECKSUM_DELIM);
    if (checksum==NULL){
        int relIx = checksum - thisSent;
        fprintf(stderr,"NMEA Checksum Delim Missing: \"%.*s\" we found: %c at possition %i\n",size,thisSent,*checksum,relIx);
        return(-2);
    }

    //test checksum
    if (testChecksum(thisSent)<0){
        fprintf(stderr,"Checksum Failed!\n");
        // return(-1);
    };

    // trimCheckSum
    *checksum = '\0';
    // loop through the array and track what part of the message we are in
    int i=0;
    char* nextSent;
    //read into an array of char arrays (so we can coppy data easily)
    // increment sent so it moves off sent start delim
    thisSent++;
    for(i =0; i<NMEA_MAX_DATA_ARRAY_SIZE; i++){
        nextSent = strchr(thisSent, NMEA_SENTENCE_DATA_DELIM);
        if (nextSent==NULL){
            break;
        }
        memcpy(outputSent[i].data(),thisSent,nextSent-thisSent);
        *nextSent = '\0'; //terminate the string
        thisSent = nextSent+1;
    };
    memcpy(outputSent[i].data(),thisSent,strlen(thisSent));

   return(i--);//return the number of bytes written
};

void NEO6M::fillMeasStruct(parsedNmeaSent& parsedSent){
gpgsaFields m;
    if(0==strcmp(parsedSent[0].data(),"GPGGA")){
        //lat/long
       lastCompleteSample.latt_deg = calcBearingInDegrees(parsedSent[m.lat.idx].data(), parsedSent[m.latB.idx].data());
       lastCompleteSample.long_deg = calcBearingInDegrees(parsedSent[m.lon.idx].data(), parsedSent[m.lonB.idx].data());
        // fix qulity
       lastCompleteSample.fixQuality=decChar2IntLUT[parsedSent[m.qual.idx][0]];
       lastCompleteSample.hdop=atof(&parsedSent[m.hdop.idx][0]);
       lastCompleteSample.tLastUpdate=atoi(&parsedSent[m.tLastUpdate.idx][0]);
        //utc
       memcpy(&lastCompleteSample.utc,parsedSent[m.t.idx].data(),strlen(parsedSent[m.t.idx].data()));
        //alt
       if (0==strcmp("M",parsedSent[m.altUnit.idx].data())){
           lastCompleteSample.alt_m = atof(parsedSent[m.alt.idx].data());
       }
       hasMeasurementCB(lastCompleteSample);
    }
    #ifdef DEBUG
    else {
        fprintf(stderr,"No matching sentance type found for: %.*s\n",(int)strlen(parsedSent[0].data()),parsedSent[0].data());
        }
    #endif
};

float NEO6M::calcBearingInDegrees(char* thisCharBearing, char* thisCharDirection){
    // the format for nmea strings is:
        //dd + mm.mmmm/60 for latitude
        // ddd + mm.mmmm/60 for longitude
    // so the bit that is consistent is the minuits so we need to figure out where that starts
    int digitCount= (thisCharBearing[4]=='.' ? 2 : 3); // two of 3 intiger bits which are in degrees
    char intPart[4];
    memcpy(intPart, thisCharBearing, digitCount);
    float deg = (float)atoi(intPart);
    deg += (atof(thisCharBearing+digitCount))/60;
    if ((*thisCharDirection == 'W') || (*thisCharDirection == 'S')){
        deg =-deg;
    }
    return deg;
};

int NEO6M::printSample(neo6mMeasurment m){
fprintf(stderr,"Lat(D)-> %.8f\n",m.latt_deg);
fprintf(stderr,"Lon(D)-> %.8f\n",m.long_deg);
fprintf(stderr,"Alt(M) -> %.3f\n",m.alt_m);
fprintf(stderr,"UTC(HHMMss.ss)-> %.*s\n",(int)sizeof(m.utc),lastCompleteSample.utc);
fprintf(stderr,"Quality-> %d\n",m.fixQuality);
fprintf(stderr,"tLastUpdate(s)-> %d\n",m.tLastUpdate);
fprintf(stderr,"hdop-> %.2f\n",m.hdop);
return(0);
}

int NEO6M::configurePort(int fd){
// adapted from:
// https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
// largely boilerplate
      struct termios tty;
      if (tcgetattr (fd, &tty) != 0){
                perror("Failed to return Uart Port attributes");
                exit(1);//
        }

        if (cfsetospeed(&tty, B9600) !=0){
                perror("Failed to set Uart port Send BaudRate");
                exit(2);//
        };

        if (cfsetispeed(&tty, B9600) !=0){
                perror("Failed to set Uart port Recieve BaudRate");
                exit(3);//
        };
        if (fcntl(fd, F_SETFL,0)<0){
           perror("fcntl returned");
        }
        /* control modes */
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        tty.c_cflag &= ~PARENB; // Clear the parity bit
        tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
        // tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
        // tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication

        //Input Modes
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        // Special handling of characters such as carriage return etc
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|ICRNL);//IGNCR ommited // disable break processing
        // cannonical read flag return on a linebreak (set to zero for non canon)
        // the echo is to do with terminals since you need to see the chars you type
        tty.c_lflag |= ICANON;                // BF local modes - no signaling chars, no echo,
        // Output modes
        tty.c_oflag = 0;                // BF no remapping, no delays
        /*
        * Im setting vmin to 1 here which means we read out bytes 1 at a
        * time which is inefficient but there is no other way to make
        * this code "real time" since the messages all have variable size
        * and come in random order so if you were buffering bytes in the
        * device, there is always a chance that the end of the message
        * doesn't get to the end of the buffer so then you need to wait
        * untill the next one comes allong before the user gets the
        * callback
            */
        tty.c_cc[VMIN]  = 1;            // set to blocking buffer at least 1 char
        tty.c_cc[VTIME] = 0.5*10;            // 0.5 seconds read timeout (deciseconds)

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); //BF input modes - shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= 0; //parity bit
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0){
                perror("Failed to Set Uart Port attributes");
                exit(4);//
        }
        return 0;//sucess
};

int NEO6M::testChecksum(char* sentance){
// largely adapted from:
  // https://github.com/craigpeacock/NMEA-GPS/blob/23f15158f124cc2b65c0468d5c690be23a449a77/gps.c#L109
	char *checksum_str;
	int returned_checksum;
	int calculated_checksum = 0;

	// Checksum is postcede by *
	checksum_str = strchr(sentance, NMEA_SENTENCE_CHECKSUM_DELIM);
	if (checksum_str != NULL){
		// Remove checksum from sentance
		// *checksum_str = '\0';

		// loop through remaining sentance to Calculate checksum, starting after $ (i = 1)
  		int lenSent = checksum_str-sentance;
		for (int i = 1; i < lenSent ; i++) {
			calculated_checksum = calculated_checksum ^ sentance[i];
		}

        //returned checksum is apparently in hex characters
		returned_checksum = hexChar2Int((char *)checksum_str+1);
		if (returned_checksum == calculated_checksum) {
			//Checksum is fine
			return(0);
		}
	} else {
#ifdef DEBUG
		fprintf(stderr,"Error: Checksum missing or NULL NMEA message\r\n");
#endif
		return(-1);
	}
	return(-1);
};

int NEO6M::hexChar2Int(char* checksumChar){
    // TODO write a test for this method should be fairly easy!!
    // convert the msb and lsb into ints and then scale msb by a factor of 16
    int msb = hexChar2IntLUT[(*checksumChar)];
    int lsb = hexChar2IntLUT[*(checksumChar+1)];
    if (msb<0){
        fprintf(stderr,"Checksum MSB char %c is not a valid hex char",msb);
        return(-1);
    };
    if (msb<0){
        fprintf(stderr,"Checksum LSB char %c is not a valid hex char",lsb);
        return(-2);
    };
    int checksum = msb*16+lsb;
    return checksum;
};

