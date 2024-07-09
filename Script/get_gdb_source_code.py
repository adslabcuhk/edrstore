import os
prefix = "https://github.com/bminor/binutils-gdb/archive/refs/tags/"
#gcc-11.2.0.tar.gz
#version_set = ["gcc-11.2.0"]
version_set = [
    "gdb-19990422",
    "gdb-19990504",
    "gdb-1999-05-10",
    "gdb-1999-05-19",
    "gdb-1999-05-25",
    "gdb-1999-06-01",
    "gdb-1999-06-07",
    "gdb-1999-06-14",
    "gdb-1999-06-21",
    "gdb-1999-06-28",
    "gdb-1999-07-05",
    "gdb-1999-07-07",
    "gdb-1999-07-12",
    "gdb-1999-07-19",
    "gdb-1999-07-26",
    "gdb-1999-08-02",
    "gdb-1999-08-09",
    "gdb-1999-08-16",
    "gdb-1999-08-23",
    "gdb-1999-08-30",
    "gdb-1999-09-08",
    "gdb-1999-09-13",
    "gdb-1999-09-21",
    "gdb-1999-09-28",
    "gdb-1999-10-04",
    "gdb-1999-10-11",
    "gdb-1999-10-18",
    "gdb-1999-10-25",
    "gdb-1999-11-01",
    "gdb-1999-11-08",
    "gdb-1999-11-16",
    "gdb-1999-12-06",
    "gdb-1999-12-07",
    "gdb-1999-12-13",
    "gdb-1999-12-21",
    "gdb-2000-01-05",
    "gdb-2000-01-10",
    "gdb-2000-01-17",
    "gdb-2000-01-24",
    "gdb-2000-01-26",
    "gdb-2000-01-31",
    "gdb-2000-02-01",
    "gdb-2000-02-02",
    "gdb-2000-02-04",
    "gdb-4_18-release",
    "gdb_4_18_2-2000-05-18-release",
    "gdb_5_0-2000-05-19-release",
    "gdb_5_1-2001-11-21-release",
    "gdb_5_1_0_1-2002-01-03-release",
    "gdb_5_1_1-2002-01-24-release",
    "gdb_5_2-2002-04-29-release",
    "gdb_5_2_1-2002-07-23-release",
    "gdb_5_3-2002-12-12-release",
    "gdb_6_0-2003-10-04-release",
    "gdb_6_1-2004-04-05-release",
    "gdb_6_1_1-20040616-release",
    "gdb_6_2-20040730-release",
    "gdb_6_3-20041109-release",
    "gdb_6_4-20051202-release",
    "gdb_6_6-2006-12-18-release",
    "gdb_6_7-2007-10-10-release",
    "gdb_6_7_1-2007-10-29-release",
    "gdb_6_8-2008-03-27-release",
    "gdb_7_0-2009-10-06-release",
    "gdb_7_0_1-2009-12-22-release",
    "gdb_7_1-2010-03-18-release",
    "gdb_7_2-2010-09-02-release",
    "gdb_7_3-2011-07-26-release",
    "gdb_7_3_1-2011-09-04-release",
    "gdb_7_4-2012-01-24-release",
    "gdb_7_4_1-2012-04-26-release",
    "gdb_7_5-2012-08-17-release",
    "gdb_7_5_1-2012-11-29-release",
    "gdb_7_6-2013-04-26-release",
    "gdb_7_6_1-2013-08-30-release",
    "gdb_7_6_2-2013-12-08-release",
    "gdb-7.7-release",
    "gdb-7.7.1-release",
    "gdb-7.8-release",
    "gdb-7.8.1-release",
    "gdb-7.8.2-release",
    "gdb-7.9.0-release",
    "gdb-7.9.1-release",
    "gdb-7.10-release",
    "gdb-7.10.1-release",
    "gdb-7.11-release",
    "gdb-7.11.1-release",
    "gdb-7.12-release",
    "gdb-7.12.1-release",
    "gdb-8.0-release",
    "gdb-8.0.1-release",
    "gdb-8.1-release",
    "gdb-8.1.1-release",
    "gdb-8.2-release",
    "gdb-8.2.1-release",
    "gdb-8.3-release",
    "gdb-8.3.1-release",
    "gdb-9.1-release",
    "gdb-9.2-release",
    "gdb-10.1-release",
    "gdb-10.2-release",
    "gdb-11.1-release",
    "gdb-11.2-release",
    "gdb-12.1-release"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/gdb_src/"

def Download(input_link):
    cmd = "wget --no-check-certificate -P " + download_output_folder + " " + input_link + ".tar.gz"
    print(cmd)
    os.system(cmd)

def Process(input_version):
    # uncompressed
    input_file = download_output_folder + input_version + ".tar.gz"
    cmd = "tar " + uncompressed_flag + " " + input_file + " -C " + download_output_folder
    print(cmd)
    os.system(cmd)
    # remove the download file
    cmd = "rm " + input_file
    print(cmd)
    os.system(cmd)

    # re-pack the folder
    repack_folder = download_output_folder + "binutils-gdb-" + input_version
#    output_file_name = repack_output_folder + input_version[0:4] + ".tar"
    output_file_name = repack_output_folder + input_version + ".tar"
    cmd = "tar " + package_flag + " " + output_file_name + " " + repack_folder
    print(cmd)
    os.system(cmd)
    cmd = "rm -rf " + repack_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":

    print version_set

    for version in version_set:
        input_link = prefix + version
        Download(input_link)
        Process(version)
