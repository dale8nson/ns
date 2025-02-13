//
//  main.cpp
//  ns
//
//  Created by Dale Hutchinson on 11/2/2025.
//

#include <algorithm>
#include <errno.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <print>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

enum class cmd { scaffold, create, add };
static map<string, cmd> command{{"scaffold", cmd::scaffold}, {"create", cmd::create}, {"add", cmd::add}};

enum class create_sub_cmd { component, page, route, layout, api_route };
static map<string, create_sub_cmd> create_sub{
    {"component", create_sub_cmd::component}, {"page", create_sub_cmd::page}, {"layout", create_sub_cmd::layout}, {"api", create_sub_cmd::api_route}};

bool in(string str, vector<string> vec) {

  return any_of(vec.cbegin(), vec.cend(), [str](string s) { return s == str; });
}

size_t index(string str, vector<string> args) {
  for (size_t i{0}; i < args.size(); i++)
    if (str == args[i])
      return i;
  return -1;
}

string join(vector<const char *> vec, string delim = " ") {
  string str{};
  for (size_t i{0}; i < vec.size(); i++) {
    str += vec[i];
    if (i < vec.size() - 1)
      str += delim;
  }
  return str;
}

vector<string> split(string str, const char delim = ' ') {
  println("{}", __FUNCTION__);
  size_t offset{0};

  vector<string> vec{};

  size_t count{0};

  for (size_t i{0}; i < str.length(); i++) {
    println("{}:{}", i, str[i]);
    if (str[i] == delim) {

      println("str.substr({}, {}): {}", offset, i, str.substr(offset, i - offset));
      string w{str.substr(offset, i - offset)};
      println("w: {}", w);
      vec.push_back(str.substr(offset, i - offset));

      offset = i;
      count++;
    }
  }

  return vec;
}

size_t end_of(string str, string subject) {
  char c{};
  string match{};
  size_t i{0};

  while (match != subject && i < str.length()){
    c = str[i]; i++;
    if(c == subject[0]) {
      match += c;
      for(size_t j{1}; (c = str[i]) == subject[j] && j < subject.length(); j++) {match += c; i++; }
      println("match: {}", match);
      if(match == subject) return i;
      match.clear();
    }
  }
  return i;
}

void insert_after(string insert_str, string &into_str, string after_str) {
  size_t p = end_of(into_str, after_str);
  println("p: {}", p);
  into_str.insert(p, insert_str);
}


string import_string(vector<string> args, string flag = "-i") {
  
  ostringstream oss{};
  map<string, vector<string>> import_map{};
  map<string, string> default_imports{};
  
  vector<string> hooks {"useState", "useEffect", "useMemo", "useRef", "useCallback", "use"};
  for(string hook: hooks) if(in(hook, args)) import_map["react"].push_back(hook);
  
  if(any_of(args.cbegin(), args.cend(), [&args, &hooks](string arg ){ return in(arg, hooks) || arg == "--client";})) oss << "\"use client\"\n";
  
  if(in(flag, args)) {
    for(size_t i{0}; i < args.size(); i++) {
      if(args[i] == flag) {
        if(args[i + 3] == "default") { default_imports[args[i + 2]] = args[i + 1]; i += 3;}
        else {import_map[args[i + 2]].push_back(args[i + 1]); i += 2; }
      }
    }
    for(pair<string, vector<string>> imp: import_map ) {
      oss << "import { " << imp.second[0];
      for(size_t i{1}; i < imp.second.size(); i++) oss << ", " << imp.second[i];
      oss << " } from \"" << imp.first << "\"\n";
    }
    for(pair<string, string> imp: default_imports) oss << "import " << imp.second << " from \"" << imp.first << "\"\n";
  }
  oss << "\n";
  
  return oss.str();
}

void create_component(vector<string> args) {
  string name = args[2];
  string cwd = fs::current_path();
  if(fstream comp{cwd + "/components/" + name + ".tsx", ios_base::out | ios_base::trunc}; comp) {
    map<string, vector<string>> import_map{};
    map<string, string> default_imports{};
    vector<string> hooks {"useState", "useEffect", "useMemo", "useRef", "useCallback", "use"};
    for(string hook: hooks) if(in(hook, args)) import_map["react"].push_back(hook);
    if(any_of(args.cbegin(), args.cend(), [&args, &hooks](string arg ){ return in(arg, hooks) || arg == "--client";})) comp << "\"use client\"\n";
    if(in("-i", args)) {
      
      for(size_t i{0}; i < args.size(); i++) {
        if(args[i] == "-i") {
          if(args[i + 3] == "default") { default_imports[args[i + 2]] = args[i + 1]; i += 3;}
          else {import_map[args[i + 2]].push_back(args[i + 1]); i += 2; }
        }
      }
      for(pair<string, vector<string>> imp: import_map ) {
        comp << "import { " << imp.second[0];
        for(size_t i{1}; i < imp.second.size(); i++) comp << ", " << imp.second[i];
        comp << " } from \"" << imp.first << "\"\n";
      }
      for(pair<string, string> imp: default_imports) comp << "import " << imp.second << " from \"" << imp.first << "\"\n";
    }
    
    comp << "\nexport ";
    if(in("--export-default", args)) comp << "default function " << name << "() {\n\n";
    else comp << "const " << name << " = () => {\n\n";
    comp << "  return (";
    string tag {""};
    if(in("--root", args)) tag = args[index("--root", args) + 1 ];
    comp << "\n    <" << tag << ">\n    </"<< tag <<">\n  )\n}\n";
    comp.close();
  }
}

void create_api_route(vector<string> args) {
  
  string cwd = fs::current_path();
  
  fs::path api_path {cwd + "/app/api" + args[2]};
  if(!fs::exists(api_path)) fs::create_directories(api_path);
  println("{}", api_path.string() + "/route.ts");
  if(fstream route{api_path.string() + "/route.ts", ios_base::out | ios_base::trunc}; route) {
    route << "import { NextRequest, NextResponse } from \"next/server\"\n";
    map<string, vector<string>> import_map{};
    map<string, string> default_imports{};
    if(in("-i", args)) {
      for(size_t i{0}; i < args.size(); i++) {
        if(args[i] == "-i") {
          if(args[i + 3] == "default") { default_imports[args[i + 2]] = args[i + 1]; i += 3;}
          else {import_map[args[i + 2]].push_back(args[i + 1]); i += 2; }
        }
      }
      for(pair<string, vector<string>> imp: import_map ) {
        route << "import { " << imp.second[0];
        for(size_t i{1}; i < imp.second.size(); i++) route << ", " << imp.second[i];
        route << " } from \"" << imp.first << "\"\n";
      }
      for(pair<string, string> imp: default_imports) route << "import " << imp.second << " from \"" << imp.first << "\"\n";
    }
    route << "\n";
    if(in("GET", args)) {
      route << 
      "export const GET = async (req: NextRequest) => {\n\n"
      "  return NextResponse.json({})\n}\n\n"
      ;
    }
    if(in("POST", args)) {
      route <<
      "export const POST = async (req: NextRequest) => {\n\n"
      "// const { } = await req.json()\n\n"
      "  return NextResponse.json({})\n}\n\n"
      ;
    }
    
    route.close();
  }
  else println("unable to create file {}", api_path.string() + "/route.ts");
}

void create_route(vector<string> args) {
  string cwd = fs::current_path();
  fs::path route_path {cwd + "app" + args[2]};
  if(!fs::exists(route_path)) fs::create_directories(route_path);
  
  if(in("--layout", args)) {
    if(fstream lo{route_path.string() + "/layout.tsx", ios_base::out | ios_base::trunc }; lo) {
      string imports = import_string(args, "-li");
      lo << "import type { Metadata } from \"next\";\n";
      lo << imports;
      lo << "export const metadata: Metadata = {\ntitle: \"\",\ndescription: \"\",\nkeywords: []\n}\n\n"
      "export default function Layout({\n  children,\n}: Readonly<{\n  children: React.ReactNode;\n"
      "}>) {\n\n  return (\n"
      "    <div className={\"min-[0px]:w-full min-[0px]:h-full\"}>\n"
      "      {children}\n    </div>\n  )\n}\n";
      
      lo.close();
    }
  }
  
  if(fstream route{route_path.string() + "/page.tsx", ios_base::out | ios_base::trunc}; route) {
    
    string imports = import_string(args);
    route << imports;
    route << "export default function Page() {\n\n"
    "  return (\n"
    "    <div className=\"w-full h-full flex flex-col justify-between items-center\">\n"
    "    </div>\n  );\n}\n"
    ;
    
    route.close();
  }
}

void write_redux_files(string cwd) {

  fs::path lib_dir{cwd + "/lib"};
  if (!fs::exists(lib_dir))
    fs::create_directory(lib_dir);

  if (fstream rs{cwd + "/lib/store.ts", ios_base::out | ios_base::trunc}; rs) {
    rs << "import { configureStore, PayloadAction, createSlice } from \"@reduxjs/toolkit\"\n\n"
          "type InitialState = {\n  selectedTab: string | null\n}\n\n"
          "const initialState: InitialState = {\n  selectedTab: null\n}\n\n"
          "const rootSlice = createSlice({\n  name: \"root\",\n  initialState,\n  reducers: {\n    },\n  },\n)\n\n"
          "export const makeStore = () => {\n"
          "  return configureStore({\n"
          "    reducer: {\n      root: rootSlice.reducer,\n     },\n  })\n}\n\n"
          "export type AppStore = ReturnType<typeof makeStore>\n"
          "export type RootState = ReturnType<AppStore[\"getState\"]>\n"
          "export type AppDispatch = AppStore['dispatch']\n\n"
          "// export const { } = rootSlice.actions\n";
    rs.close();
  }

  if (fstream hk{cwd + "/lib/hooks.ts", ios_base::out | ios_base::trunc}; hk) {
    hk << "\"use client\"\n"
          "import { useDispatch, useSelector, useStore } from \"react-redux\"\n"
          "import type { RootState, AppDispatch, AppStore } from \"./store\"\n\n"
          "export const useAppDispatch = useDispatch.withTypes<AppDispatch>()\n"
          "export const useAppSelector = useSelector.withTypes<RootState>()\n"
          "export const useAppStore = useStore.withTypes<AppStore>()";
    hk.close();
  }

  fs::path component_dir{cwd + "/components"};
  if (!fs::exists(component_dir))
    fs::create_directory(component_dir);

  if (fstream sp{cwd + "/components/StoreProvider.tsx", ios_base::out | ios_base::trunc}; sp) {
    sp << "\"use client\"\n"
          "import { useRef } from \"react\"\n"
          "import { Provider } from \"react-redux\"\n"
          "import { makeStore, AppStore } from \"../lib/store\"\n\n"
          "export const StoreProvider = ({\n  children,\n}: {\n  children: React.ReactNode\n}) => {\n"
          "  const storeRef = useRef<AppStore>(undefined)\n  if (!storeRef.current) {\n"
          "    storeRef.current = makeStore()\n  }\n\n"
          "  return <Provider store={storeRef.current}>{children}</Provider>\n}\n";
    sp.close();
  }
  
  
  
}

void add_redux(string cwd) {
  write_redux_files(cwd);
  
  string str{};
  if(fstream rl{cwd + "/app/layout.tsx", ios_base::out | ios_base::in}; rl) {
    rl.unsetf(ios_base::skipws);
    char c{};
    while ((rl >> c) && !rl.eof()) str += c;
    
    insert_after("import { StoreProvider } from \"@/components/StoreProvider\"\n", str, "globals.css\"\n");
    size_t p = end_of(str, "return (\n");
    println("p: {}", p);
    p += end_of(str.substr(p), ">\n");
    println("p: {}", p);
    str.insert(p, "        <StoreProvider>\n");
    insert_after("        </StoreProvider>\n", str,"</body>\n" );
    
    rl.close();
  }
  
  println("{}", str);

  
  if(fstream rl{cwd + "/app/layout.tsx", ios_base::out | ios_base::trunc}; rl) {
    rl << str;
  }

  system("npm i react-redux @reduxjs/toolkit");
}

void scaffold(string cwd, vector<string> args) {

  fs::path components_path{cwd + "/components"};
  fs::directory_entry components_folder{components_path};
  if (!fs::exists(components_path))
    fs::create_directory(components_path);

  string project_name{};
  if (in("new", args)) {
    project_name = {args[index("new", args) + 1]};
    system(format("npx create-next-app {} --yes", project_name).c_str());
    std::filesystem::current_path(cwd + "/" + project_name);
    cwd = fs::current_path();
  }

  ostringstream installs{};
  installs.unsetf(ios_base::skipws);
  installs << "npm install ";

  bool redux = in("--redux", args);
  if (redux)
    installs << "react-redux "
             << "@reduxjs/toolkit ";
  bool query = in("--react-query", args);
  if (query)
    installs << " @tanstack/react-query";

  fs::path app_dir{cwd + "/app"};

  fstream rl{cwd + "/app/layout.tsx", ios_base::out | ios_base::trunc};
  rl << "import type { Metadata } from \"next\"\n"
     << "import \"./globals.css\"\n";
  if (redux)
    rl << "import { StoreProvider } from \"@/components/StoreProvider\"\n\n\n";
  else rl << "\n\n";
  rl << "export const metadata: Metadata = {\ntitle: \"\",\ndescription: \"\",\nkeywords: []\n}\n\n"
        "export default function RootLayout({\n  children,\n}: Readonly<{\n  children: React.ReactNode\n"
        "}>) {\nreturn (\n    <html lang=\"en\" className=\"min-[0px]:w-screen min-[0px]:h-screen\">\n";

  if (redux)
    rl << "      <StoreProvider>\n";
  rl << "      <body\n        className={\"min-[0px]:w-full min-[0px]:h-full md:w-screen md:h-screen antialiased\"}\n"
        "      >\n        {children}\n      </body>\n";
  if (redux)
    rl << "      </StoreProvider>\n";
  rl << "    </html>\n  );\n}";
  rl.close();

  if (fstream hp{cwd + "/app/page.tsx", ios_base::out | ios_base::trunc}; hp) {
    hp << "export default function Home() {\n\n"
          "  return (\n"
          "    <div className=\"w-full h-full flex flex-col justify-between items-center\">\n"
          "    </div>\n  );\n}\n";

    hp.close();
  }

  if (redux)
    write_redux_files(cwd);

  int err = system(installs.str().c_str());

  if (err)
    perror(strerror(errno));
}


int main(int argc, const char *argv[]) {

  vector<string> args;
  for (size_t i{1}; i < argc; i++) {
    args.push_back(string{argv[i]});
  }

  const string cwd{fs::current_path()};

  const string public_dir{cwd + "/public"};
  const string route_dir{cwd + "/app/api"};
  const string components_dir{cwd + "/components"};

  fs::path public_path{public_dir};
  fs::directory_entry public_folder{public_path};

  string cmd_str{args[0]};

  switch (command[cmd_str]) {
  case cmd::scaffold:
    scaffold(cwd, args);
    break;
  case cmd::create: {
    create_sub_cmd sub = create_sub[args[1]];
    switch (sub) {
    case create_sub_cmd::component:
        create_component(args);
      break;
    case create_sub_cmd::page:
      break;
    case create_sub_cmd::layout:
      break;
    case create_sub_cmd::route:
        create_route(args);
      break;
    case create_sub_cmd::api_route:
        create_api_route(args);
      break;
    }
    break;
  }
  case cmd::add:
      string sub_cmd{args[1]};
      if(sub_cmd == "redux") add_redux(cwd);
    break;
  }

  return 0;
}
