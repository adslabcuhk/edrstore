import os
prefix = "https://github.com/torvalds/linux/archive/refs/tags/"

version_set = []

def pre_version_set(base_version, rc_begin, rc_end):
    global version_set
    version_set += [base_version]
    for i in range(rc_begin, rc_end + 1):
        version_set += [base_version + "-rc" + str(i)]

uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/linux_archives/"

def Download(input_link):
    cmd = "wget --no-check-certificate -P " + download_output_folder + " " + input_link + ".tar.gz"
    print(cmd)
    os.system(cmd)

def Process(input_version):
    # uncompressed
    input_file = download_output_folder + "v" + input_version + ".tar.gz"
    cmd = "tar " + uncompressed_flag + " " + input_file + " -C " + download_output_folder
    print(cmd)
    os.system(cmd)
    # remove the download file
    cmd = "rm " + input_file
    print(cmd)
    os.system(cmd)

    # re-pack the folder
    repack_folder = download_output_folder + "linux-"+ input_version
#    output_file_name = repack_output_folder + input_version[0:4] + ".tar"
    output_file_name = repack_output_folder + "v" + input_version + ".tar"
    cmd = "tar " + package_flag + " " + output_file_name + " " + repack_folder
    print(cmd)
    os.system(cmd)
    cmd = "rm -rf " + repack_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":
    pre_version_set("3.0", 1, 5)

    print version_set

    for version in version_set:
        input_link = prefix + "v" + version
        Download(input_link)
        Process(version)