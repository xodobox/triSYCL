/* RUN: %{execute}%s | %{filecheck} %s
   CHECK: Result:
   CHECK-NEXT: 6 8 11

   A convoluted example of pipes using 2 different short-live queues.

   Since the queues disappear after each submit() in this example, the
   run-time wait for completion of the first queue, which has to send
   all the data in the pipe before the second queue start.

   If the pipe is not big enough, there is a dead-lock: the sending
   kernel fill the pipe but the consumer kernel cannot start in the
   meantime
*/
#include <CL/sycl.hpp>
#include <iostream>
#include <iterator>

constexpr size_t N = 3;
using Vector = float[N];

int main() {
  Vector a = { 1, 2, 3 };
  Vector b = { 5, 6, 8 };
  Vector c;

  {
    // Create buffers from a & b vectors
    cl::sycl::buffer<float> A { std::begin(a), std::end(a) };
    cl::sycl::buffer<float> B { std::begin(b), std::end(b) };

    // A buffer of N float using the storage of c
    cl::sycl::buffer<float> C { c, N };

    //  A pipe of float with at least N elements to avoid the dead-lock
    cl::sycl::pipe<float> P { N };

    // Launch the producer to stream A to the pipe on its own queue
    cl::sycl::queue {}.submit([&](cl::sycl::handler &cgh) {
      // Get write access to the pipe
      auto p = P.get_access<cl::sycl::access::write>(cgh);
      // Get read access to the data
      auto ka = A.get_access<cl::sycl::access::read>(cgh);

      cgh.single_task<class producer>([=] {
          for (int i = 0; i != N; i++)
            // Try to write to the pipe up to success
            while (!(p.write(ka[i])))
              ;
        });
      }); //< The first queue goes out-of-scope: wait for completion!

    /* Launch the consumer that adds the pipe stream with B to C on
       its own queue */
    cl::sycl::queue {}.submit([&](cl::sycl::handler &cgh) {
      // Get read access to the pipe
      auto p = P.get_access<cl::sycl::access::read>(cgh);

      // Get access to the input/output buffers
      auto kb = B.get_access<cl::sycl::access::read>(cgh);
      auto kc = C.get_access<cl::sycl::access::write>(cgh);

      cgh.single_task<class consumer>([=] {
          for (int i = 0; i != N; i++) {
            /* Declare a variable of the same type as what the pipe
               can deal (a good example of single source advantage)
            */
            decltype(p)::value_type e;
            // Try to read from the pipe up to success
            while (!(p.read(e)))
              ;
            kc[i] = e + kb[i];
          }
        });
      }); //< The second queue goes out-of-scope: wait for completion!
  } /*< End scope, so we wait for the buffers to be unused, in this
      case there is nothing to wait for */

  std::cout << std::endl << "Result:" << std::endl;
  for (auto e : c)
    std::cout << e << " ";
  std::cout << std::endl;
}
