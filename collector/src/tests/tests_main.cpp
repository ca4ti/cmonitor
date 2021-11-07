//------------------------------------------------------------------------------
// GTest unit tests MAIN file
//------------------------------------------------------------------------------

#include "../cgroups.h"
#include "../logger.h"
#include "../output_frontend.h"
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <malloc.h>

std::string get_unit_test_abs_dir()
{
    char buff[PATH_MAX + 1];
    if (getcwd(buff, sizeof(buff)) == NULL)
        exit(123);
    char actualpath[PATH_MAX + 1];
    char* ptr = realpath(buff, actualpath);

    return std::string(ptr) + "/";
}

void prepare_sample_dir(std::string kernel_test, unsigned int sampleIdx)
{
    std::string orig_sample_abs_dir = get_unit_test_abs_dir() + kernel_test + "/sample" + std::to_string(sampleIdx);
    std::string orig_sample_tarball = get_unit_test_abs_dir() + kernel_test + "/sample" + std::to_string(sampleIdx) + "/sample" + std::to_string(sampleIdx) + ".tar.gz";
    std::string current_sample_abs_dir = get_unit_test_abs_dir() + kernel_test + "/current-sample";

    char buff[1024];
    snprintf(buff, 1024, "/usr/bin/tar -C %s -xf %s", orig_sample_abs_dir.c_str(), orig_sample_tarball.c_str());
    printf("Executing now: %s\n", buff);
    system(buff);

    printf("Adjusting symlink %s\n", current_sample_abs_dir.c_str());
    unlink(current_sample_abs_dir.c_str());
    symlink(orig_sample_abs_dir.c_str(), current_sample_abs_dir.c_str());
}

TEST(CGroups, Read3Samples)
{
    CMonitorOutputFrontend actual_output("test1.json");
    CMonitorCollectorAppConfig cfg;
    cfg.m_strCGroupName = "docker/5ccb1395eef093a837e302c52f8cb633cc276ea7d697151ecc34187db571a3b2";

    CMonitorLogger::instance()->enable_debug();
    CMonitorLogger::instance()->init_error_output_file("stdout");

    double elapsed_sec = 0.1;
    std::set<std::string> allowedStats;

    std::string kernel_test = "centos7-Linux-3.10.0-x86_64";
    std::string current_sample_abs_dir = get_unit_test_abs_dir() + kernel_test + "/current-sample";
    prepare_sample_dir(kernel_test, 1);

    // allocate the class under test:
    CMonitorCgroups t(&cfg, &actual_output);
    t.cgroup_init( // force newline
        current_sample_abs_dir + "/sys/fs/cgroup/memory", // force newline
        current_sample_abs_dir + "/sys/fs/cgroup/cpu,cpuacct", // force newline
        current_sample_abs_dir + "/sys/fs/cgroup/cpuset");

    // start feeding fixed, test data
    actual_output.pheader_start();
    actual_output.push_header();
    actual_output.psample_array_start();
    for (unsigned int i = 0; i < 3; i++) {
        printf("\n\n\n");
        prepare_sample_dir(kernel_test, i+1);

        actual_output.psample_start();
        t.cgroup_proc_cpuacct(elapsed_sec, true /* emit JSON */);
        t.cgroup_proc_memory(allowedStats);

        // FIXME FIXME: currently this method does not read from our unit test folder
        t.cgroup_proc_tasks(elapsed_sec, cfg.m_nOutputFields /* emit JSON */, false /* do not include threads */);
        //t.cgroup_proc_tasks(elapsed_sec, cfg.m_nOutputFields /* emit JSON */, true /* do include threads */);

        actual_output.push_current_sample();
    }
    actual_output.psample_array_end();

    // TODO: compare produced JSON with expected JSON

    ASSERT_EQ(CMonitorLogger::instance()->get_num_errors(), 0);
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Make uses of freed and uninitialized memory known.
    mallopt(M_PERTURB, 42);

    // RUN ALL GTESTS
    testing::InitGoogleTest(&argc, argv);
    if (RUN_ALL_TESTS() != 0)
        return 1;

    std::cout << "All GoogleTest testsuites passed." << std::endl;

    return 0;
}
