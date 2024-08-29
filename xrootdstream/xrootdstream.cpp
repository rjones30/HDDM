#include "xrootdstream.hpp"

#include <exception>
#include <sstream>
#include <fstream>

xrootdStreambuf::xrootdStreambuf(const std::string& url, size_t buffersize)
 : url_(url),
   lookback_(3),
   readahead_(3),
   buffer_index_(-1),
   buffersize_(buffersize),
   verbose_(0)
{
   buffer_.push_back(new stream_block(0, buffersize_));
   //xrdfs_ = new XrdCl::FileSystem(XrdCl::URL(url));
   xrdfile_ = new XrdCl::File();
   buffer_.back()->resp_ = xrdfile_->Open(url, XrdCl::OpenFlags::Read);
   if (! buffer_.back()->resp_.IsOK()) {
      delete xrdfile_;
      xrdfile_ = 0;
      std::stringstream errmsg;
      errmsg << "xrootdStreambuf constructor - open request failed"
             << " for " << url;
      throw std::runtime_error(errmsg.str());
   }
   buffer_.back()->resp_ = xrdfile_->Read(buffer_.back()->offset_,
                                          buffer_.back()->size_,
                                          buffer_.back()->buf_,
                                          buffer_.back()->bytes_read_);
   char *buf = (char*)buffer_.back()->buf_;
   size_t len = buffer_.back()->bytes_read_;
   setg(buf, buf, buf + len);
   buffer_index_ = 0;
   advance();
}

xrootdStreambuf::~xrootdStreambuf() {
   std::deque<stream_block*>::iterator iter;
   for (iter = buffer_.begin(); iter != buffer_.end(); ++iter) {
      if ((*iter)->reader_ != 0) {
         (*iter)->reader_->join();
         delete (*iter)->reader_;
      }
      delete *iter;
   }
   if (xrdfile_ != 0) {
      XrdCl::XRootDStatus resp = xrdfile_->Close();
      if (! buffer_.back()->resp_.IsOK()) {
         //std::cerr << "XrdCl::File::Close returns error on file "
         //          << url_ << std::endl;
      }
      delete xrdfile_;
   }
}

int xrootdStreambuf::advance() {
   int nblocks(0);
   while (buffer_.size() - buffer_index_ < readahead_) {
      std::streampos offset = buffer_.back()->offset_;
      std::streamoff size = buffer_.back()->size_;
      buffer_.push_back(new stream_block(offset + size, buffersize_));
      buffer_.back()->reader_ = new std::thread(stream_block::background_fill,
                                                buffer_.back(), 
		                                xrdfile_,
                                                url_);
      if (verbose_ > 0) {
         std::cout << "advance asks for another " << buffersize_ << " bytes "
                   << "starting at offset " << offset << std::endl;
      }
      nblocks++;
   }
   return nblocks;
}

void xrootdStreambuf::stream_block::background_fill(stream_block *block,
		                                    XrdCl::File *xrdfile,
                                                    const std::string url) {
   block->resp_ = xrdfile->Read(block->offset_, block->size_,
		                block->buf_,  block->bytes_read_);
   if (! block->resp_.IsOK()) {
      std::stringstream errmsg;
      errmsg << "xrootdStreambuf::background_fill - read request for "
             << url << " returned error.";
      throw std::runtime_error(errmsg.str());
   }
}

int xrootdStreambuf::underflow() {
   if (verbose_ > 0) {
      std::cout << "underflow entry with buffer_index_ " << buffer_index_ 
                << " and stream offset " << buffer_.back()->offset_ << std::endl;
      std::cout << "   eback=" << (void*)eback() << std::endl
                << "    gptr=" << (void*)gptr() << std::endl
                << "   egptr=" << (void*)egptr() << std::endl;
   }
   if (++buffer_index_ == buffer_.size())
      return std::streambuf::underflow();
   buffer_[buffer_index_]->reader_->join();
   delete buffer_[buffer_index_]->reader_;
   buffer_[buffer_index_]->reader_ = 0;
   char *buf = (char*)buffer_[buffer_index_]->buf_;
   size_t len = buffer_[buffer_index_]->bytes_read_;
   if (len == 0) {
      return std::streambuf::underflow();
   }
   setg(buf, buf, buf + len);
   while (buffer_index_ > lookback_) {
      delete buffer_.front();
      buffer_.pop_front();
      --buffer_index_;
   }
   advance();
   if (verbose_ > 0) {
      std::cout << "underflow exit with buffer_index_ " << buffer_index_ 
                << " and stream offset " << buffer_.back()->offset_ << std::endl;
      std::cout << "   eback=" << (void*)eback() << std::endl
                << "    gptr=" << (void*)gptr() << std::endl
                << "   egptr=" << (void*)egptr() << std::endl;
   }
   return (int)(unsigned char)*buf;
}

void xrootdStreambuf::setg(char *gbeg, char *gcurr, char *gend) {
   if (verbose_ > 0) {
      std::cout << "setg with gbeg=" << (void*)gbeg
                << ", gcurr=" << (void*)gcurr
                << ", gend=" << (void*)gend << std::endl;
   }
   std::streambuf::setg(gbeg, gcurr, gend);
}
