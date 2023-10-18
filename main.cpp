#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz)
{
    return path(data, data + sz);
}

bool Preprocess(ifstream& source, const path& file, ofstream& destination, const vector<path>& include_directories)
{
    static regex relative(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex absolute(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");

    string line;
    int index = 0;
    while (getline(source, line))
    {
        ++index;

        smatch local, global;
        if (regex_match(line, local, relative) || regex_match(line, global, absolute))
        {
            string filename = global.empty() ? local[1] : global[1];
            path include_path = global.empty() ? file.parent_path() / filename : "";

            ifstream include;
            include.open(include_path);

            for (const auto& directory : include_directories)
            {
                if (include.is_open())
                    break;
                include_path = directory / filename;
                include.open(include_path);
            }

            if (include.is_open())
            {
                if (Preprocess(include, include_path, destination, include_directories))
                    continue;
                else
                    return false;
            }

            cout << "unknown include file " << filename << " at file " << file.string() << " at line " << index << endl;

            return false;
        }

        destination << line << endl;
    }

    return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    ifstream source(in_file);
    if (!source)
        return false;

    ofstream destination(out_file);
    if (!destination)
        return false;

    return Preprocess(source, in_file, destination, include_directories);
}


string GetFileContents(string file)
{
    ifstream stream(file);

    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

void Test() 
{
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
            "#include \"dir1/b.h\"\n"
            "// text between b.h and c.h\n"
            "#include \"dir1/d.h\"\n"
            "\n"
            "int SayHello() {\n"
            "    cout << \"hello, world!\" << endl;\n"
            "#   include<dummy.txt>\n"
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
            "#include \"subdir/c.h\"\n"
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
            "#include <std1.h>\n"
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
            "#include \"lib/std2.h\"\n"
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    ostringstream test_out;
    test_out << "// this comment before include\n"
        "// text from b.h before include\n"
        "// text from c.h before include\n"
        "// std1\n"
        "// text from c.h after include\n"
        "// text from b.h after include\n"
        "// text between b.h and c.h\n"
        "// text from d.h before include\n"
        "// std2\n"
        "// text from d.h after include\n"
        "\n"
        "int SayHello() {\n"
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() 
{
    Test();
}