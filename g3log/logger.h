#ifndef __PRINT_SINK__
#define __PRINT_SINK__

#include <string>
#include <iostream>
#include <stdexcept>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

#include <std2_make_unique.hpp>
#include <g2log.hpp>
#include <g2logworker.hpp>

class scoped_out {
  std::ostream& _out_type;
  std::streambuf* _old_cout;
public:
  explicit scoped_out(std::ostream& out_type, std::stringstream* buffer)
     : _out_type(out_type)
     , _old_cout(_out_type.rdbuf()) {
      _out_type.rdbuf(buffer->rdbuf());
     }
      
  virtual ~scoped_out() {
      _out_type.rdbuf(_old_cout);
   }
};

class print_sink {
public:
   std::stringstream buffer;
   std::unique_ptr<scoped_out> scope_ptr;

   print_sink() : scope_ptr(std2::make_unique<scoped_out>(std::cout, &buffer)) {}

   void clear() { buffer.str(""); }
   std::string string() { return buffer.str(); }
   void print(g2::LogMessageMover msg) { std::clog << msg.get()./*message*/toString() /*<< std::flush*/; }
   virtual ~print_sink() final {}
   static std::unique_ptr<print_sink> create() { return std::unique_ptr<print_sink>(new print_sink);}
};

class logger {
	g2::DefaultFileLogger *logger_;
public:
	logger(const std::string& prefix, const std::string& directory) {		
		if (!boost::filesystem::exists(directory)) {
			boost::filesystem::create_directories(directory);
		}

		logger_ = new g2::DefaultFileLogger(prefix, directory);
		g2::initializeLogging(logger_->worker.get());
		logger_->worker->addSink(print_sink::create(), &print_sink::print);
	};

	~logger() { delete logger_; };
};

#endif

