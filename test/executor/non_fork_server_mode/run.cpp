#define BOOST_TEST_MODULE native_linux_executor.run
#define BOOST_TEST_DYN_LINK
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Executor/NativeLinuxExecutor.hpp>
#include <Feedback/InplaceMemoryFeedback.hpp>
#include <Feedback/PUTExitReasonType.hpp>
#include <Exceptions.hpp>
#include <Utils/Common.hpp>
#include <Utils/Filesystem.hpp>
#include <Utils/Which.hpp>
#include <boost/scope_exit.hpp>

// NativeLinuxExecutor::Run() の正常系テスト
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRun) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      { fuzzuf::utils::which( fs::path( "tee" ) ).c_str(), output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );

  // Invoke NativeLinuxExecutor::Run()
  size_t INPUT_LENGTH = 14;
  std::string input( "Hello, World!" );
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  
  // Check normality
  // (1) 正常実行されたこと → feedbackのexit_reason が PUTExitReasonType::FAULT_NONE であることを確認する
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_NONE );

  // (2) 標準入力によってファズが受け渡されたこと → 標準入力と同じ内容がファイルに保存されたことを確認する
  BOOST_CHECK( fs::exists( output_file_path ) );
  BOOST_CHECK_EQUAL( fs::file_size( output_file_path ), input.length() );
  int output_file = Util::OpenFile( output_file_path.native(), O_RDONLY);
  BOOST_CHECK_LE( input.length(), INPUT_LENGTH );
  char output_file_buf[INPUT_LENGTH];
  BOOST_CHECK_GT( output_file, -1 );
  Util::ReadFile( output_file, static_cast<void *>(output_file_buf), input.length() );
  BOOST_CHECK_EQUAL( strncmp( 
      output_file_buf,
      reinterpret_cast< const char* >( input.c_str() ),
      input.length() 
    ), 0 );
  Util::CloseFile(output_file);
}

// 正常終了するプロセスがFAULT_NONEになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunOk) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      //{ "/usr/bin/tee", output_file_path.native() },
      { TEST_BINARY_DIR "/executor/ok", output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_NONE );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     0 );
}

// 異常終了(非ゼロな引数によるexit)するプロセスがFAULT_NONEになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunFail) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      { TEST_BINARY_DIR "/executor/fail", output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_NONE );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     0 );
}

// abort()するプロセスがFAULT_CRASHになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunAbort) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      { TEST_BINARY_DIR "/executor/abort", output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );

  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_CRASH );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     SIGABRT );
}

// segmentaton faultするプロセスがFAULT_CRASHになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunSegmentationFault) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      { TEST_BINARY_DIR "/executor/segmentation_fault", output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_CRASH );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     SIGSEGV );
}

// 終了しないプロセスがFAULT_TMOUTになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunNeverExit) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
      { TEST_BINARY_DIR "/executor/never_exit", output_file_path.native() },
      1000,
      10000,
      false,
      path_to_write_seed,
      true,
      true,
      NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_TMOUT );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     SIGKILL );
}

// 存在しない実行可能バイナリの実行がFAULT_ERRORになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunNotExists) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
    { TEST_BINARY_DIR "/executor/not_exists", output_file_path.native() },
    1000,
    10000,
    false,
    path_to_write_seed,
    true,
    true,
    NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_ERROR );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     0 );
}

// 実行権限がついていないファイルの実行がFAULT_ERRORになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunPermissionDenied) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
    { TEST_DICTIONARY_DIR "/test.dict", output_file_path.native() },
    1000,
    10000,
    false,
    path_to_write_seed,
    true,
    true,
    NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_ERROR );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     0 );
}

// 実行権限が付いているが実際には実行できないファイルの実行がFAULT_ERRORになることを確認する
BOOST_AUTO_TEST_CASE(NativeLinuxExecutorNativeRunNotExecutable) {
  // Setup root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END

  // Create input/output dirctory
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  BOOST_CHECK_EQUAL( fs::create_directory( output_dir ), true );

  // Setup output file path
  auto output_file_path = output_dir / "result";
  std::cout << "[*] output_file_path = " + output_file_path.native() << std::endl;
  
  // Create executor instance
  auto path_to_write_seed = output_dir / "cur_input";
  NativeLinuxExecutor executor( 
    { TEST_SOURCE_DIR "/executor/not_executable", output_file_path.native() },
    1000,
    10000,
    false,
    path_to_write_seed,
    true,
    true,
    NativeLinuxExecutor::CPUID_DO_NOT_BIND
  );
  BOOST_CHECK_EQUAL( executor.stdin_mode, true );
  
  // Invoke NativeLinuxExecutor::Run()
  std::string input;
  executor.Run(
    reinterpret_cast< const u8* >( input.c_str() ),
    input.size()
  );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().exit_reason, 
                     PUTExitReasonType::FAULT_ERROR );
  BOOST_CHECK_EQUAL( executor.GetExitStatusFeedback().signal, 
                     0 );
}
