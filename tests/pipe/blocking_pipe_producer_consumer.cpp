/* RUN: %{execute}%s | %{filecheck} %s
   CHECK: Result:
   CHECK-NEXT: 6 8 11

   Extend a pipe program example from Andrew Richards
   https://cvs.khronos.org/bugzilla/show_bug.cgi?id=14215
*/
#include <CL/sycl.hpp>
#include <iostream>
#include <iterator>

constexpr size_t N = 3;
using Vector = float[N];

int main() {
  Vector va = { 1, 2, 3 };
  Vector vb = { 5, 6, 8 };
  Vector vc;

  {
    // Create buffers from a & b vectors
    cl::sycl::buffer<float> ba { std::begin(va), std::end(va) };
    cl::sycl::buffer<float> bb { std::begin(vb), std::end(vb) };

    // A buffer of N float using the storage of vc
    cl::sycl::buffer<float> bc { vc, N };

    // A pipe of 2 float elements
    cl::sycl::pipe<float> p { 2 };

    // Create a queue to launch the kernels
    cl::sycl::queue q;

    // Launch the producer to stream A to the pipe
    q.submit([&](cl::sycl::handler &cgh) {
      // Get write access to the pipe
      auto kp = p.get_access<cl::sycl::access::write,
                             cl::sycl::access::blocking_pipe>(cgh);
      // Get read access to the data
      auto ka = ba.get_access<cl::sycl::access::read>(cgh);

      cgh.single_task<class producer>([=] {
          for (int i = 0; i != N; i++)
            kp.write(ka[i]);
        });
      });

    // Launch the consumer that adds the pipe stream with B to C
    q.submit([&](cl::sycl::handler &cgh) {
      // Get read access to the pipe
      auto kp = p.get_access<cl::sycl::access::read,
                             cl::sycl::access::blocking_pipe>(cgh);

      // Get access to the input/output buffers
      auto kb = bb.get_access<cl::sycl::access::read>(cgh);
      auto kc = bc.get_access<cl::sycl::access::write>(cgh);

      cgh.single_task<class consumer>([=] {
          for (int i = 0; i != N; i++) {
            kc[i] = kp.read() + kb[i];
          }
        });
      });
  } //< End scope for the queue and the buffers, so wait for completion

  std::cout << std::endl << "Result:" << std::endl;
  for (auto e : vc)
    std::cout << e << " ";
  std::cout << std::endl;
}
