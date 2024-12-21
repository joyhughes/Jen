#include <vector>
#include <variant>
#include <string>
#include <functional>

template< class U > class joy_fn;

std::variant< joy_fn<bool>, joy_fn<int>, joy_fn<float>, joy_fn<std::string> > any_joy_fn;

template< class T > std::string next_name() {
    static int count = 0;
    return std::string( "fn" ) + std::to_string( count++ );
}

template< class U > class joy_fn {
    
    std::function< T( T ) > fn;
    std::vector< std::reference_wrapper< any_joy_fn > > harnesses; // References to harnesses in functor within 
    U val;
    std::string name;

    public:

    std::string get_name() { return name; }
    void set_name( const std::string& name_init ) { name = name_init; }
    bool name_is( const std::string& query ) { return name == query; }
    void operator () ( element_context& context );

    U  operator *  () { return  val; }
    U* operator -> () { return &val; }
    joy_fn< U >& operator = ( const U& u ) { val = u; return *this; } // value assignment

    void set_function( const &std::function< T( T ) > fn ) { this->fn = fn; }

    joy_fn( const std::string& name_init = next_name< U >() );
    joy_fn( const U& val_init, const std::string& name_init = next_name< U >() );
    joy_fn( const joy_fn& h );  // Copy constructor
    joy_fn(joy_fn&& h) noexcept; // Move constructor
    joy_fn<U>& operator=(const joy_fn& h); // Copy assignment operator
    joy_fn<U>& operator=(joy_fn&& h) noexcept; // Move assignment operator

    ~joy_fn();
};
