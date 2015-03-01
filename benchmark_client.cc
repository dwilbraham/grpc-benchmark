#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "benchmark.pb.h"

using grpc::ChannelArguments;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using benchmark::Count;
using benchmark::Data;
using benchmark::Empty;


class BenchmarkClient {
 public:
  BenchmarkClient(std::shared_ptr<ChannelInterface> channel)
      : stub_(benchmark::Benchmark::NewStub(channel)) {
  }

  void ResetCount() {
    ClientContext context;
    Empty empty;
    Count count;
    Status status = stub_->ResetCount(&context, empty, &count);
    if (status.IsOk()) {
      //std::cout << "  Count was: " << count.count() << std::endl;
    }
  }

  void GetData(uint64_t loop_count) {
    Data data;
    Data ret_data;
    data.set_type("test");
    data.set_value("test2");

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    for(uint64_t i = 0; i < loop_count; ++i) {
      ClientContext context;
      Status status = stub_->GetData(&context, data, &ret_data);
      if (!status.IsOk()) {
        throw std::exception();
      }
    }

    end = std::chrono::system_clock::now();
 
    std::chrono::duration<double> elapsed_seconds = end-start;
    double avg = ret_data.count() / elapsed_seconds.count();
 
    std::cout << "  Got\t" << ret_data.count() << "\tevents in\t"
              << elapsed_seconds.count() << "s\t"
              << "Average of\t" << avg << "\tevents/sec\n";
  }

  void GetDataStream(uint64_t loop_count) {
    Data data;
    Data ret_data;
    data.set_type("test");
    data.set_value("test2");
    data.set_count(loop_count);

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    ClientContext context;
    std::unique_ptr<ClientReader<Data> > reader(
        stub_->GetDataStream(&context, data));
    uint64_t i = 0;
    while (reader->Read(&ret_data)) {
      ++i;
    }
    Status status = reader->Finish();

    end = std::chrono::system_clock::now();
 
    std::chrono::duration<double> elapsed_seconds = end-start;
    double avg = i / elapsed_seconds.count();
 
    std::cout << "  Got\t" << i << "\tevents in\t"
              << elapsed_seconds.count() << "s\t"
              << "Average of\t" << avg << "\tevents/sec\n";
  }
/*
  void ListFeatures() {
    Rectangle rect;
    Feature feature;
    ClientContext context;

    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    std::cout << "Looking for features between 40, -75 and 42, -73"
              << std::endl;

    std::unique_ptr<ClientReader<Feature> > reader(
        stub_->ListFeatures(&context, rect));
    int count = 0;
    while (reader->Read(&feature)) {
      ++count;
      std::cout << "Found feature called "
                << feature.name() << " at "
                << feature.location().latitude()/kCoordFactor_ << ", "
                << feature.location().latitude()/kCoordFactor_ << std::endl;
    }
    std::cout << "Found " << count << " features" << std::endl;
    Status status = reader->Finish();
    if (status.IsOk()) {
      std::cout << "ListFeatures rpc succeeded." << std::endl;
    } else {
      std::cout << "ListFeatures rpc failed." << std::endl;
    }
  }

  void RecordRoute() {
    Point point;
    RouteSummary stats;
    ClientContext context;
    const int kPoints = 10;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> feature_distribution(
        0, feature_list_.size() - 1);
    std::uniform_int_distribution<int> delay_distribution(
        500, 1500);

    std::unique_ptr<ClientWriter<Point> > writer(
        stub_->RecordRoute(&context, &stats));
    for (int i = 0; i < kPoints; i++) {
      const Feature& f = feature_list_[feature_distribution(generator)];
      std::cout << "Visiting point "
                << f.location().latitude()/kCoordFactor_ << ", "
                << f.location().longitude()/kCoordFactor_ << std::endl;
      if (!writer->Write(f.location())) {
        // Broken stream.
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(
          delay_distribution(generator)));
    }
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.IsOk()) {
      std::cout << "Finished trip with " << stats.point_count() << " points\n"
                << "Passed " << stats.feature_count() << " features\n"
                << "Travelled " << stats.distance() << " meters\n"
                << "It took " << stats.elapsed_time() << " seconds"
                << std::endl;
    } else {
      std::cout << "RecordRoute rpc failed." << std::endl;
    }
  }

  void RouteChat() {
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<RouteNote, RouteNote> > stream(
        stub_->RouteChat(&context));

    std::thread writer([stream]() {
      std::vector<RouteNote> notes{
        MakeRouteNote("First message", 0, 0),
        MakeRouteNote("Second message", 0, 1),
        MakeRouteNote("Third message", 1, 0),
        MakeRouteNote("Third message2", 1, 0),
        MakeRouteNote("Fourth message", 0, 0)};
      for (const RouteNote& note : notes) {
        std::cout << "Sending message " << note.message()
                  << " at " << note.location().latitude() << ", "
                  << note.location().longitude() << std::endl;
        stream->Write(note);
      }
      stream->WritesDone();
    });

    RouteNote server_note;
    while (stream->Read(&server_note)) {
      std::cout << "Got message " << server_note.message()
                << " at " << server_note.location().latitude() << ", "
                << server_note.location().longitude() << std::endl;
    }
    writer.join();
    Status status = stream->Finish();
    if (!status.IsOk()) {
      std::cout << "RouteChat rpc failed." << std::endl;
    }
  }
*/
  void Shutdown() { stub_.reset(); }

 private:
  void print_data(const Data& data) {
    std::cout << "  type:\t"    << data.type();
    std::cout << "\t\tvalue:\t" << data.value();
    std::cout << "\t\tcount:\t" << data.count();
    std::cout << std::endl;
  }

  std::unique_ptr<benchmark::Benchmark::Stub> stub_;
};

int main(int argc, char** argv) {
  grpc_init();

  BenchmarkClient guide(
      grpc::CreateChannelDeprecated("10.240.248.67:50051", ChannelArguments())
      );

  std::cout << "-------------- GetData --------------" << std::endl;
  guide.GetData(1);
  guide.ResetCount();
  guide.GetData(100);
  guide.ResetCount();
  guide.GetData(10000);
  guide.ResetCount();
  std::cout << "-------------- GetDataStream --------------" << std::endl;
  guide.GetDataStream(1);
  guide.ResetCount();
  guide.GetDataStream(100);
  guide.ResetCount();
  guide.GetDataStream(10000);
  guide.ResetCount();
  guide.GetDataStream(1000000);
  guide.ResetCount();
/*  std::cout << "-------------- RecordRoute --------------" << std::endl;
  guide.RecordRoute();
  std::cout << "-------------- RouteChat --------------" << std::endl;
  guide.RouteChat();
*/
  guide.Shutdown();

  grpc_shutdown();
}
