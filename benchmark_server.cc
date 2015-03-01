#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "benchmark.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using benchmark::Count;
using benchmark::Data;
using benchmark::Empty;
using std::chrono::system_clock;


class BenchmarkImpl final : public benchmark::Benchmark::Service {
 public:
  explicit BenchmarkImpl() : 
    data_request_count_(0)
  {}

  Status ResetCount(ServerContext* context, const Empty* empty,
                    Count* count) override {
    count->set_count(data_request_count_);
    data_request_count_ = 0;
    return Status::OK;
  }

  Status GetData(ServerContext* context, const Data* data_in,
                    Data* data_out) override {
    data_out->CopyFrom(*data_in);
    data_out->set_count(++data_request_count_);
    return Status::OK;
  }

  Status GetDataStream(ServerContext* context, const Data* data_in,
                       ServerWriter<Data>* writer) override {
    Data data_out;
    data_out.CopyFrom(*data_in);
    for (uint64_t i = 0; i < data_in->count(); ++i) {
      data_out.set_count(++data_request_count_);
      writer->Write(data_out);
    }
    return Status::OK;
  }
/*
  Status RecordRoute(ServerContext* context, ServerReader<Point>* reader,
                     RouteSummary* summary) override {
    Point point;
    int point_count = 0;
    int feature_count = 0;
    float distance = 0.0;
    Point previous;

    system_clock::time_point start_time = system_clock::now();
    while (reader->Read(&point)) {
      point_count++;
      if (!GetFeatureName(point, feature_list_).empty()) {
        feature_count++;
      }
      if (point_count != 1) {
        distance += GetDistance(previous, point);
      }
      previous = point;
    }
    system_clock::time_point end_time = system_clock::now();
    summary->set_point_count(point_count);
    summary->set_feature_count(feature_count);
    summary->set_distance(static_cast<long>(distance));
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time);
    summary->set_elapsed_time(secs.count());

    return Status::OK;
  }

  Status RouteChat(ServerContext* context,
                   ServerReaderWriter<RouteNote, RouteNote>* stream) override {
    std::vector<RouteNote> received_notes;
    RouteNote note;
    while (stream->Read(&note)) {
      for (const RouteNote& n : received_notes) {
        if (n.location().latitude() == note.location().latitude() &&
            n.location().longitude() == note.location().longitude()) {
          stream->Write(n);
        }
      }
      received_notes.push_back(note);
    }

    return Status::OK;
  }
*/
 private:

  uint64_t data_request_count_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  BenchmarkImpl service;

  ServerBuilder builder;
  builder.AddPort(server_address);
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  grpc_init();

  RunServer();

  grpc_shutdown();
  return 0;
}
