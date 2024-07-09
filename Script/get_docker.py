#script for downloading & processing the cassandra docker container traces
from inspect import trace
import os
home_dir = "/home/usr" # need to change to your home dir

version_set = [
"2",
"2.0",
"2.0.14",
"2.0.15",
"2.0.16",
"2.0.17",
"2.1",
"2.1.5",
"2.1.6",
"2.1.7",
"2.1.8",
"2.1.9",
"2.1.10"
"2.1.11",
"2.1.12",
"2.1.13",
"2.1.14",
"2.1.15",
"2.1.16",
"2.1.17",
"2.1.18",
"2.1.19",
"2.1.20",
"2.1.21",
"2.1.22",
"2.2",
"2.2.0",
"2.2.1",
"2.2.2",
"2.2.3",
"2.2.4",
"2.2.5",
"2.2.6",
"2.2.7",
"2.2.8",
"2.2.9",
"2.2.10",
"2.2.11",
"2.2.12",
"2.2.13",
"2.2.14",
"2.2.15",
"2.2.16",
"2.2.19"
"3",
"3.0",
"3.0.0",
"3.0.1",
"3.0.2",
"3.0.3",
"3.0.4",
"3.0.5",
"3.0.6",
"3.0.7",
"3.0.8",
"3.0.9",
"3.0.10",
"3.0.11",
"3.0.12",
"3.0.13",
"3.0.14",
"3.0.15",
"3.0.16",
"3.0.17",
"3.0.18",
"3.0.19",
"3.0.20",
"3.0.21",
"3.0.22",
"3.0.23",
"3.0.24",
"3.0.25",
"3.0.26",
"3.0.27",
"3.1",
"3.1.1",
"3.2",
"3.2.1",
"3.3",
"3.4",
"3.5",
"3.6",
"3.7",
"3.8",
"3.9",
"3.10",
"3.11",
"3.11.0",
"3.11.1",
"3.11.2",
"3.11.3",
"3.11.4",
"3.11.5",
"3.11.6",
"3.11.7",
"3.11.8",
"3.11.9",
"3.11.10",
"3.11.11",
"3.11.12",
"3.11.13"
"4",
"4.0",
"4.0.0",
"4.0.1",
"4.0.2",
"4.0.3",
"4.0.4",
"4.0.5"
]

package_flag = "-cvf"
download_output_folder = os.path.join(home_dir, "cassandra_src/download/") # for output folder
repack_output_folder = os.path.join(home_dir, "cassandra_src/packed/") # for output folder

#bash download-frozen-image-v2.sh tst couchbase:enterprise-6.6.1
def Download(input_version):
    cmd = "bash download-frozen-image-v2.sh " + download_output_folder + " " + "cassandra:" + input_version 
    print(cmd)
    os.system(cmd)
    # decompress all layer.tar in the sub-folder
    for sub_file in os.listdir(download_output_folder):
        sub_dir = os.path.join(download_output_folder, sub_file)
        if (os.path.isdir(sub_dir)):
            # check whether it has the sub-folder
            for sub_sub_file in os.listdir(sub_dir):
                if (sub_sub_file == "layer.tar"):
                    sub_sub_file_path = os.path.join(sub_dir, sub_sub_file)
                    cmd = "tar -xvf " + sub_sub_file_path + " -C " + sub_dir
                    print(cmd)
                    os.system(cmd)
                    cmd = "rm " + sub_sub_file_path
                    print(cmd)
                    os.system(cmd)

def PackTar(input_version):
    output_file_name = os.path.join(repack_output_folder, input_version + ".tar")
    cmd = "tar " + package_flag + " " + output_file_name + " " + download_output_folder
    print(cmd)
    os.system(cmd)

    delete_folder = os.path.join(download_output_folder, "*")
    cmd = "rm -rf " + delete_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":
    for version in version_set:
        Download(version)
        PackTar(version)
