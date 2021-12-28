#include <iostream>
#include <string>
#include <Python.h>
int main() {
  Py_Initialize();
  auto site = PyImport_ImportModule( "site" );
  if ( !site ) return 1;
  auto getsitepackages = PyObject_GetAttrString( site, "getsitepackages" );
  if ( !getsitepackages ) return 1;
  auto getsitepackages_args = Py_BuildValue( "()" );
  if ( !getsitepackages_args ) return 1;
  auto path_list = PyObject_CallObject( getsitepackages, getsitepackages_args );
  if ( !path_list ) return 1;
  const auto list_size = PyList_Size( path_list );
  if( list_size == 0 ) return 1;
  auto path = PyList_GetItem( path_list, list_size - 1 );
  if( !path ) return 1;
  auto encoded_path = PyUnicode_EncodeLocale( path, nullptr );
  if( !encoded_path ) return 1;
  std::cout << PyBytes_AS_STRING( encoded_path ) << std::flush;
  Py_Finalize();
}
