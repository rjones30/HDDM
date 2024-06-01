#include "httpstream.hpp"

#include <exception>
#include <sstream>
#include <fstream>

httpStreambuf::httpStreambuf(const std::string& url, size_t buffersize)
 : url_(url),
   lookback_(3),
   readahead_(3),
   buffer_index_(-1),
   buffersize_(buffersize),
   verbose_(0)
{
   buffer_.push_back(new stream_block(0, buffersize_));
   buffer_.back()->resp_ = cpr::Get(cpr::Url{url},
		           cpr::VerifySsl(0),
                           cpr::ReserveSize(buffersize),
                           cpr::Range{0, buffersize_ - 1});
   if (buffer_.back()->resp_.status_code != 206) {
      std::stringstream errmsg;
      errmsg << "httpStreambuf constructor - get request for "
             << url << " returned error with HTTP response code " 
             << buffer_.back()->resp_.status_line << " : "
             << buffer_.back()->resp_.reason;
      throw std::runtime_error(errmsg.str());
   }
   char *buf = (char*)buffer_.back()->resp_.text.c_str();
   size_t len = buffer_.back()->resp_.text.size();
   setg(buf, buf, buf + len);
   buffer_index_ = 0;
   advance();
}

httpStreambuf::~httpStreambuf() {
   std::deque<stream_block*>::iterator iter;
   for (iter = buffer_.begin(); iter != buffer_.end(); ++iter) {
      if ((*iter)->reader_ != 0) {
         (*iter)->reader_->join();
         delete (*iter)->reader_;
      }
      delete *iter;
   }
}

int httpStreambuf::advance() {
   int nblocks(0);
   while (buffer_.size() - buffer_index_ < readahead_) {
      std::streampos offset = buffer_.back()->offset_;
      std::streamoff size = buffer_.back()->size_;
      buffer_.push_back(new stream_block(offset + size, buffersize_));
      buffer_.back()->reader_ = new std::thread(stream_block::background_fill,
                                                buffer_.back(), 
                                                url_);
      if (verbose_ > 0) {
         std::cout << "advance asks for another " << buffersize_ << " bytes "
                   << "starting at offset " << offset << std::endl;
      }
      nblocks++;
   }
   return nblocks;
}

void httpStreambuf::stream_block::background_fill(stream_block *block,
                                                  const std::string url) {
   block->resp_ = cpr::Get(cpr::Url{url},
		           cpr::VerifySsl(0),
                           cpr::ReserveSize(block->size_),
                           cpr::Range{block->offset_,
                                      block->offset_ + block->size_ - 1}
                          );
   if (block->resp_.status_code != 206 && block->resp_.status_code != 416) {
      std::stringstream errmsg;
      errmsg << "httpStreambuf::background_fill - get request for "
             << url << " returned error with HTTP response code " 
             << block->resp_.status_line << " : "
             << block->resp_.reason;
      throw std::runtime_error(errmsg.str());
   }
}

int httpStreambuf::underflow() {
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
   char *buf = (char*)buffer_[buffer_index_]->resp_.text.c_str();
   size_t len = buffer_[buffer_index_]->resp_.text.size();
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

void httpStreambuf::setg(char *gbeg, char *gcurr, char *gend) {
   if (verbose_ > 0) {
      std::cout << "setg with gbeg=" << (void*)gbeg
                << ", gcurr=" << (void*)gcurr
                << ", gend=" << (void*)gend << std::endl;
   }
   std::streambuf::setg(gbeg, gcurr, gend);
}
