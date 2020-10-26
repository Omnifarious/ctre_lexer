#include <functional>
#include <type_traits>
#include <unordered_set>

namespace myns {

struct empty { };

}

namespace std {

template <class T>
class hash : public enable_if<
   is_same<size_t, decltype(declval<const T>().hash()),
           ::myns::empty>::type
{
  public:
   size_t hash(T const &v) const { return v.hash(); }
};

}

class Point {
 public:
   int x;
   int y;

   ::std::size_t hash() const { return x ^ y; }
};


auto test()
{
   ::std::unordered_set<Point> s;
   s.add(Point{1, 2});
}
