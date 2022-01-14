#include <cstdio>
#include <vector>
#include <sparsepp/spp.h>

using spp::sparse_hash_map;
using namespace std;

class FileSerializer
{
public:
    // serialize basic types to FILE
    // -----------------------------
    template <class T>
    bool operator()(FILE *fp, const T& value)
    {
        return fwrite((const void *)&value, sizeof(value), 1, fp) == 1;
    }

    template <class T>
    bool operator()(FILE *fp, T* value)
    {
        return fread((void *)value, sizeof(*value), 1, fp) == 1;
    }

    // serialize std::string to FILE
    // -----------------------------
    bool operator()(FILE *fp, const string& value)
    {
        const size_t size = value.size();
        return (*this)(fp, size) && fwrite(value.c_str(), size, 1, fp) == 1;
    }

    bool operator()(FILE *fp, string* value)
    {
        size_t size;
        if (!(*this)(fp, &size))
            return false;
        char* buf = new char[size];
        if (fread(buf, size, 1, fp) != 1)
        {
            delete [] buf;
            return false;
        }
        new (value) string(buf, (size_t)size);
        delete[] buf;
        return true;
    }

    // serialize std::vector<T, A> to FILE
    // -----------------------------------
    template <class T, class A = std::allocator<T>>
    bool operator()(FILE *fp, const std::vector<T, A>& value)
    {
        const size_t size = value.size();
        if (!(*this)(fp, size))
            return false;
        for (size_t i=0; i<size; ++i)
            if (!(*this)(fp, value[i]))
                return false;
        return true;
    }

    template <class T, class A = std::allocator<T>>
    bool operator()(FILE *fp, std::vector<T, A>* value)
    {
        size_t size;
        if (!(*this)(fp, &size))
            return false;
        new (value) std::vector<T, A>(size);
        for (size_t i=0; i<size; ++i)
            if (!(*this)(fp, &(*value)[i]))
                return false;
        return true;
    }

    // serialize spp::sparse_hash_map<K, V, H, E, A> to FILE
    // -----------------------------------------------------
    template <class K, class V, 
              class H = spp::spp_hash<K>, 
              class E = std::equal_to<K>, 
              class A = SPP_DEFAULT_ALLOCATOR<std::pair<const K, V> > >
    bool operator()(FILE *fp, const spp::sparse_hash_map<K, V, H, E, A>& value)
    {
        return const_cast<spp::sparse_hash_map<K, V, H, E, A> &>(value).serialize(*this, fp);
    }

    template <class K, class V, 
              class H = spp::spp_hash<K>, 
              class E = std::equal_to<K>, 
              class A = SPP_DEFAULT_ALLOCATOR<std::pair<const K, V> > >
    bool operator()(FILE *fp, spp::sparse_hash_map<K, V, H, E, A> *value)
    {
        new (value) spp::sparse_hash_map<K, V, H, E, A>();
        return value->unserialize(*this, fp);
    }

    // serialize std::pair<const A, B> to FILE - needed for maps
    // ---------------------------------------------------------
    template <class A, class B>
    bool operator()(FILE *fp, const std::pair<const A, B>& value)
    {
        return (*this)(fp, value.first) && (*this)(fp, value.second);
    }

    template <class A, class B>
    bool operator()(FILE *fp, std::pair<const A, B> *value)
    {
        return (*this)(fp, (A *)&value->first) && (*this)(fp, &value->second);
    }
};

int main(int, char* [])
{
    sparse_hash_map<string, int> age{ { "John", 12 }, {"Jane", 13 }, { "Fred", 8 } };

    // serialize age hash_map to "ages.dmp" file
    FILE *out = fopen("ages.dmp", "wb");
    age.serialize(FileSerializer(), out);
    fclose(out);

    sparse_hash_map<string, int> age_read;

    // read from "ages.dmp" file into age_read hash_map
    FILE *input = fopen("ages.dmp", "rb");
    age_read.unserialize(FileSerializer(), input);
    fclose(input);

    // print out contents of age_read to verify correct serialization
    for (auto& v : age_read)
        printf("age_read: %s -> %d\n", v.first.c_str(), v.second);
}
