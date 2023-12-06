#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}
static  size_t SHUT_RDWR=2;
void Writer::push( string data )
{
  if(this->ByteStreamState==SHUT_RDWR){
  set_error();
  return;
  }
  uint64_t datasize(data.size());
  if(this->headroom_<datasize){
  set_error();
  datasize=this->headroom_;
  data.resize(datasize);
  }
  this->buffer+=data;
  this->buffered+=datasize;
  this->pushed+=datasize;
  this->headroom_-=datasize;
}

void Writer::close()
{
  if(this->ByteStreamState==SHUT_RDWR){
    return;
  }
  this->ByteStreamState=SHUT_RDWR;
}

void Writer::set_error()
{

  // unsigned int WriteOFF_0
  // unsigned int Overflow_1
  // unsigned int ReadOFF_2
  this->errorsize++;
  // string WriteOFF="Unable to write because write has been turned off";
  // string Overflow="Data exceeds the maximum limit";
  // string ReadOFF="Unable to read because read has been turned off";
}

bool Writer::is_closed() const
{
 if(this->ByteStreamState==SHUT_RDWR){
  return true;
  }
  return false;
}

uint64_t Writer::available_capacity() const
{
  return this->headroom_;
}

uint64_t Writer::bytes_pushed() const
{
  return this->pushed;
}

string_view Reader::peek() const
{
  return std::string_view(this->buffer);
}

bool Reader::is_finished() const
{
   if(this->ByteStreamState==SHUT_RDWR&&buffered==0){
    return true;
  }
  return false;
}

bool Reader::has_error() const
{
  if(this->errorsize!=0){
    return true;
  }
  return false;
}

void Reader::pop( uint64_t len )
{
  if(len<1){
    return;
  }
  else if(len>this->buffered)
  {
    len=this->buffered;
  }
   this->buffer.erase(0, len);
   this->buffered-=len;
   this->headroom_+=len;
   this->poped+=len;
}

uint64_t Reader::bytes_buffered() const
{
  return this->buffered;
}

uint64_t Reader::bytes_popped() const
{
  return this->poped;
}
