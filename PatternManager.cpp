// vim:ts=2 sw=2:
#include "PatternManager.h"

PatternManager::PatternManager(FlashMemory * mem) {
  this->flash = mem;
  this->patternCount = 0;
  this->lastSavedPattern = -1;
  this->testPatternActive = false;
}

void PatternManager::loadPatterns() {
  this->flash->readBytes(0x100,(byte *)this->patterns,sizeof(PatternMetadata)*PatternManager::MAX_PATTERNS);
  PatternMetadata * ptr = this->patterns;
  for (int i=0; i<MAX_PATTERNS; i++) {
    if (ptr->address == 0xffffffff) {
      this->patternCount = i;
      return;
    }
    ptr++;
  }
  this->patternCount = PatternManager::MAX_PATTERNS;
}

void PatternManager::resetPatternsToDefault() {
  this->clearPatterns();

  PatternMetadata newpat;
  char patternName[] = "Default";
  memcpy(newpat.name, patternName, strlen(patternName)+1);
  newpat.len = 1*3*50;
  newpat.frames = 50;
  newpat.flags = 0;
  newpat.fps = 10;
  int led = 0;
  for (int i=0; i<10; i++) {
    this->buf[led*3+0] = i*25;
    this->buf[led*3+1] = 0;
    this->buf[led*3+2] = 0;
    led++;
  }
  for (int i=0; i<10; i++) {
    this->buf[led*3+0] = 250-i*25;
    this->buf[led*3+1] = i*25;
    this->buf[led*3+2] = 0;
    led++;
  }
  for (int i=0; i<10; i++) {
    this->buf[led*3+0] = 0;
    this->buf[led*3+1] = 250-i*25;
    this->buf[led*3+2] = i*25;
    led++;
  }
  for (int i=0; i<10; i++) {
    this->buf[led*3+0] = i*25;
    this->buf[led*3+1] = i*25;
    this->buf[led*3+2] = 250;
    led++;
  }
  for (int i=0; i<10; i++) {
    this->buf[led*3+0] = 250-i*25;
    this->buf[led*3+1] = 250-i*25;
    this->buf[led*3+2] = 250-i*25;
    led++;
  }

  byte patindex = this->saveLedPatternMetadata(&newpat);
  this->saveLedPatternBody(patindex,0,(byte*)this->buf,newpat.len);
}

void PatternManager::clearPatterns() {
  this->flash->erasePage(0x100);
  this->flash->erasePage(0x200);
  this->patternCount = 0;
  this->selectedPattern = 0xff;
  lastSavedPattern = -1;
}

void PatternManager::selectPattern(byte n) {
    Serial.print("select pattern called: ");
    Serial.println(n);
    if (this->patternCount == 0) {
      this->selectedPattern = -1;
      return;
    }

    if (n >= this->patternCount) return;

    this->selectedPattern = n;

    PatternMetadata * pat = &this->patterns[n];
    
    this->currentFrame = 0;
    this->lastFrameTime = 0;
}

void PatternManager::deletePattern(byte n) {
  this->patternCount--;
  for (int i=n; i<this->patternCount; i++) {
    memcpy(&this->patterns[i],&this->patterns[i+1],sizeof(PatternMetadata));
  }

  byte * ptr = (byte*)(&this->patterns[this->patternCount]);
  for (int i=0; i<sizeof(PatternMetadata); i++) {
    ptr[i] = 0xff;
  }
  this->flash->writeBytes(0x100+sizeof(PatternMetadata)*n,(byte *)(&this->patterns[n]),(this->patternCount-n+1)*sizeof(PatternMetadata));

  if (n <= this->selectedPattern) {
    selectPattern(this->selectedPattern-1);
  }

  if (this->patternCount == 0) this->selectedPattern = -1;
}

uint32_t PatternManager::findInsertLocation(uint32_t len) {
  for (int i=0; i<this->patternCount; i++) {
    uint32_t firstAvailablePage = ((this->patterns[i].address + this->patterns[i].len) & 0xffffff00) + 0x100;

    if (i == this->patternCount - 1) return i+1; //last pattern

    uint32_t spaceAfter = this->patterns[i+1].address - firstAvailablePage;
    if (spaceAfter > len) {
      return i+1;
    }
  }
  return 0;
}

byte PatternManager::saveLedPatternMetadata(PatternMetadata * pat) {
  byte insert = findInsertLocation(pat->len);
  if (insert == 0) {
      pat->address = 0x300;
  } else {
      //address is on the page after the previous pattern
      pat->address = ((this->patterns[insert-1].address + this->patterns[insert-1].len) & 0xffffff00) + 0x100;
  }

  for (int i=this->patternCount; i>insert; i--) {
    memcpy(&this->patterns[i],&this->patterns[i-1],sizeof(PatternMetadata));
  }
  this->patternCount ++;

  memcpy(&this->patterns[insert],pat,sizeof(PatternMetadata));
  this->flash->writeBytes(0x100+sizeof(PatternMetadata)*insert,(byte *)(&this->patterns[insert]),(this->patternCount-insert)*sizeof(PatternMetadata));

  return insert;
}

void PatternManager::saveLedPatternBody(int pattern, uint32_t patternStartPage, byte * payload, uint32_t len) {
  PatternMetadata * pat = &this->patterns[pattern];

  uint32_t writeLocation = pat->address + patternStartPage*0x100;
  this->flash->writeBytes(writeLocation,payload,len);
}

void PatternManager::saveTestPattern(PatternMetadata * pat) {
  byte insert = findInsertLocation(pat->len);
  if (insert == 0) {
      pat->address = 0x300;
  } else {
      //address is on the page after the previous pattern
      pat->address = ((this->patterns[insert-1].address + this->patterns[insert-1].len) & 0xffffff00) + 0x100;
  }

  memcpy(&this->testPattern,pat,sizeof(PatternMetadata));
}

void PatternManager::saveTestPatternBody(uint32_t patternStartPage, byte * payload, uint32_t len) {
  uint32_t writeLocation = testPattern.address + patternStartPage*0x100;
  this->flash->writeBytes(writeLocation,payload,len);
}

void PatternManager::showTestPattern(bool show) {
  this->testPatternActive = show;
  this->currentFrame = 0;
  this->lastFrameTime = 0;
}

int PatternManager::getTotalBlocks() {
  return PatternManager::NUM_PAGES;
}

int PatternManager::getUsedBlocks() {
  int blocksUsed = 0;
  for (int i=0; i<this->patternCount;i++) {
    uint32_t usedPages = this->patterns[i].len / 0x100;
    if (this->patterns[i].len % 0x100 != 0) usedPages++;

    blocksUsed += usedPages;
  }

  return blocksUsed;
}

int PatternManager::getAvailableBlocks() {
  return getTotalBlocks() - getUsedBlocks();
}

int PatternManager::getPatternCount() {
  return this->patternCount;
}

int PatternManager::getSelectedPattern() {
  return this->selectedPattern;
}

int PatternManager::getCurrentFrame() {
  return this->currentFrame;
}

int PatternManager::getPatternIndexByName(const char * name) {
  for (int i=0; i<this->patternCount; i++) {
    if (strcmp(this->patterns[i].name,name) == 0) {
      return i;
    }
  }
  return -1;
}

bool PatternManager::isTestPatternActive() {
  return this->testPatternActive;
}

PatternManager::PatternMetadata * PatternManager::getActivePattern() {
  return &this->patterns[this->getSelectedPattern()];
}

void PatternManager::syncToFrame(int frame) {
  this->currentFrame = frame;
  this->lastFrameTime = 0;
}

bool PatternManager::loadNextFrame(LEDStrip * strip) {
  if (this->patternCount == 0) {
    for (int i=0; i<strip->getLength(); i++) { //clear strip
      strip->setPixel(i,0,0,0);
    }
    return true;
  }

  PatternMetadata * active = this->getActivePattern();
  if (this->testPatternActive) active = &this->testPattern;
  if (millis() - this->lastFrameTime < 1000  / active->fps) return false; //wait for a frame based on fps
  this->lastFrameTime = millis();
  uint32_t width = active->len / (active->frames * 3); //width in pixels of the pattern
  uint32_t startAddress = active->address + (width * 3 * this->currentFrame);

  byte * bbuf = (byte*)this->buf;

//  Serial.print("Pattern Frame: ");
//  Serial.print(startAddress,HEX);
//  Serial.print(" ");
//  Serial.print(width*3);
//  Serial.println();
//  Serial.flush();
  this->flash->readBytes(startAddress,bbuf,width*3);
//  Serial.println("finished reading pattern");
  
  for (int i=0; i<strip->getLength(); i++) {
    strip->setPixel(i, bbuf[(3*(i % width))+0],bbuf[(3*(i % width))+1],bbuf[(3*(i % width))+2] );
  }
//  Serial.println("finished setting pixels");

  this->currentFrame += 1;
  if (this->currentFrame >= active->frames) {
    this->currentFrame = 0;
  }

//  Serial.println(ESP.getFreeHeap());

  return true;
}

/*
{
  'patterns':[
    {
      'index':index,
      'name':'Strip name',
      'address':addy,
      'length':len,
      'frame':frames,
      'flags':flags,
      'fps':fps
    }
  ],
  'selectedPattern':5,
  'brightness':50,
  'memory':{
    'used':100,
    'free':100,
    'total':100
  }
}
*/
int PatternManager::serializePatterns(char * buf, int bufferSize) {
  PatternManager::PatternMetadata * pat;
  char * ptr = buf;
  int size;

  size = snprintf(ptr,bufferSize,"[");
  ptr += size;
  bufferSize -= size;

  for (int i=0; i<this->getPatternCount(); i++) {
    //Serial.print("pattern loop: ");
    //Serial.print(i);
    //Serial.print("size: ");
    //Serial.println(bufferSize);
    if (i != 0) {
      size = snprintf(ptr,bufferSize,",");
      ptr += size;
      bufferSize -= size;
    }
    pat = &this->patterns[i];

    size = snprintf(ptr,bufferSize,"{\"index\":%d,\"name\":\"%s\",\"address\":%d,\"length\":%d,\"frames\":%d,\"flags\":%d,\"fps\":%d}",i,pat->name,pat->address,pat->len,pat->frames,pat->flags,pat->fps);
    ptr += size;
    bufferSize -= size;
  }

  size = snprintf(ptr,bufferSize,"]");
  ptr += size;
  bufferSize -= size;

  return ptr - buf;
}


void PatternManager::jsonPatterns(JsonArray& arr) {
  for (int i=0; i<this->getPatternCount(); i++) {
    JsonObject& json = arr.createNestedObject();
    json["index"] = i;
    json["name"] = this->patterns[i].name;
    json["address"] = this->patterns[i].address;
    json["length"] = this->patterns[i].len;
    json["frames"] = this->patterns[i].frames;
    json["flags"] = this->patterns[i].flags;
    json["fps"] = this->patterns[i].fps;
  }
}
