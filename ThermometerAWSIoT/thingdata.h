#include "FS.h"

uint8_t* bitmap1;
boolean hasbitmap = false;

// utility to load FS data into a char array
char * getCharsFromFS(String filename) {

  File stringfile = SPIFFS.open(filename, "r");
  if ( ! stringfile ) {
    Serial.println(F("unable to open file"));
  }
  int filesize = stringfile.size();

  Serial.print(F("File size: "));
  Serial.println(filesize);
  
  char * charstring = new char[filesize];
  stringfile.readBytes((char *) charstring, filesize);
  stringfile.close();

  return charstring;
}

// general helper function to get a URL and, if changed, update the FS with the contents
boolean copyUrlToFile(String url, String filename) {
  HTTPClient http;
  String etag = "";
  // this sets up the headers that are captured during a request, others are ignored
  const char * headerkeys[] = {"ETag","Last-Modified"};

  if ( filename.length() < 2 || filename.length() > 24 ) {
    Serial.print(filename);
    Serial.println(F(" is an invalid file name "));
    return false;
  }
  String tempfilename = String("/tmp") + filename;
  String etagfilename = String("/etag") + filename;

  Serial.print(F("Requesting URL: "));
  Serial.println(url);
  
  //
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  // ask http to track these headers
  http.collectHeaders(headerkeys, headerkeyssize );

  // OK, now go
  http.begin(url);

 
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.println(F("connection error"));
    return false;
  }

  if ( httpCode != HTTP_CODE_OK ) {
    Serial.print(F("Error code: "));
    Serial.println( httpCode );
    return false;
  }

  if ( http.hasHeader("ETag") ) {
    etag = http.header("ETag");
    Serial.print(F("ETAG at URL is "));
    Serial.println(etag);
    Serial.print(F("Last-Modified: "));
    Serial.println(http.header("Last-Modified"));

    // compare etag
    File etagfile = SPIFFS.open(etagfilename, "r");
    if ( etagfile ) {
      String oldetag = etagfile.readString();
      etagfile.close();
      Serial.print(F("ETAG of file is "));
      Serial.println(oldetag);
      
      if (etag.length() && oldetag.compareTo(etag) == 0) {
        Serial.print(filename);
        Serial.println(F(" has not changed!"));
        return true;
      }
      // new file, continue, remember to update the etag when done
    } // else brand new file
  }

  // cross fingers we get something to insert, create temp file
  File newfile = SPIFFS.open( tempfilename, "w" );
  if ( !newfile ) {
    Serial.println( F("file open failed") );
    http.end();
    return false;
  }

  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  int len = http.getSize();

  // create buffer for read
  uint8_t buff[128] = { 0 };

  // get stream
  // get tcp stream
  WiFiClient * stream = http.getStreamPtr();

  // read all data
  while ( http.connected() && (len > 0 || len == -1) ) {
    size_t size = stream->available();

    if (size) {
      // read up to 128 bytes
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

      // write it to Sfile
      newfile.write(buff, c);
      // display for debugging
      Serial.write(buff, c);

      if (len > 0) {
         len -= c;
      }
    }
    delay(1);
  }

  Serial.println();
  Serial.println("[HTTP] connection closed or file end.");

  unsigned int filelength = newfile.size();
  
  Serial.print(F("Got data: "));
  Serial.println(filelength);
  newfile.close();

  if ( SPIFFS.exists(filename) ) {
    Serial.println(F("deleting old file"));
    SPIFFS.remove(filename);
  }
  Serial.println(F("Renaming file"));
  SPIFFS.rename(tempfilename, filename);

  // update etag
  if (etag) {
    File updatedetagfile = SPIFFS.open(etagfilename, "w");
    if ( updatedetagfile ) {
      updatedetagfile.print(etag);
      updatedetagfile.close();
      Serial.print(F("New etag is "));
      Serial.println(etag);
    }
  }
  
  return true;
}

// fetch recipe data from S3 and update the storage
void setup_recipe_data() {
  Serial.println(F("setup recipe data "));

  copyUrlToFile(RECIPES_URL, F("/recipes.json"));

  // copyUrlToFile("http://s3.amazonaws.com/thing-data/yogurt.bin", "/yogurt.bin");
}

// utility to load FS data into a bitmap
uint8_t * getBitmapFromFS(String filename, uint16_t &w, uint16_t &h) {

  if (hasbitmap) {
    Serial.println("Returning already created bitmap");
    return bitmap1;
  }
  // this might not be necessary or we can use a list of existing data
  String fileurl = String(THING_DATA) + filename;
  copyUrlToFile(fileurl, filename);
  
  File bitmapfile = SPIFFS.open(filename, "r");
  if ( ! bitmapfile ) {
    Serial.println(F("unable to open file"));
  }
  int filesize = bitmapfile.size();
  filesize -= 6; // header is 6 bytes

  Serial.print(F("File size: "));
  Serial.println(filesize);

  // get w & h from header
  bitmapfile.seek(2, SeekSet);
  w = (uint16_t) bitmapfile.read();
  Serial.println(w);
  bitmapfile.seek(4, SeekSet);
  h = (uint16_t) bitmapfile.read();
  Serial.println(h);
  //w = 200;
  //h = 133;
  bitmapfile.seek(6, SeekSet);
  //char raw[filesize];
  
  bitmap1 = new uint8_t[filesize];
  bitmapfile.readBytes((char *) bitmap1, filesize);
  bitmapfile.close();
  //Serial.printf("Dimensions %d x %d\n", w, h);
  //Serial.println(raw);

  // TODO cache these in an array!!!
  //uint8_t * bitmap = new uint8_t[filesize];
  //memcpy(bitmap, (uint8_t*)raw, filesize);

  hasbitmap = true;
  return bitmap1;
}

