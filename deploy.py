import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime

import colorama


def log(message, level='default', end="\n"):
    if level == 'error':
        color = colorama.Fore.RED
    elif level == 'warning':
        color = colorama.Fore.YELLOW
    elif level == 'success':
        color = colorama.Fore.GREEN
    elif level == 'info':
        color = colorama.Fore.BLUE
    else:
        color = ''

    print(color + message, end=end)
    print(colorama.Style.RESET_ALL, end="")


def main() -> int:
    if platform.system() == 'Windows':
        return deploy_win()
    elif platform.system() == 'Darwin':
        return deploy_osx()
    else:
        return -1


def deploy_win_portable(windeployqt, build_folder, output_folder, version, libcaesium_path, winsparkle_path,
                        imageformats_path):
    log('Run deploy for PORTABLE', 'warning')
    output_package_folder = os.path.join(output_folder, "caesium-image-compressor-" + version + "-win")
    log('Making output package folder...', 'info', " ")
    try:
        os.makedirs(output_package_folder)
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    executable_path = os.path.join(
        build_folder, "Caesium Image Compressor.exe")
    log('Deploying executable...', 'info', " ")
    deploy_result = subprocess.run(
        [windeployqt, executable_path, "--dir", output_package_folder, "--verbose=0"])

    try:
        deploy_result.check_returncode()
        shutil.copy(executable_path, os.path.join(output_package_folder, "Caesium Image Compressor.exe"))
        log('OK', 'success')
    except subprocess.CalledProcessError:
        log('FAIL', 'error')
        raise

    log('Copying additional libraries...', 'info', " ")
    try:
        shutil.copy(libcaesium_path, os.path.join(output_package_folder, "caesium.dll"))
        shutil.copy(winsparkle_path, os.path.join(output_package_folder, "WinSparkle.dll"))
        # shutil.copy(os.path.join(imageformats_path, "qicns.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qtga.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qwbmp.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qwebp.dll"), os.path.join(output_package_folder, "imageformats"))
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Compressing package...', 'info', " ")
    try:
        shutil.make_archive(os.path.join(output_folder, "caesium-image-compressor-" + version + "-win"), 'zip',
                            output_folder, "caesium-image-compressor-" + version + "-win")
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise


def deploy_win_setup(windeployqt, source_folder, build_folder, output_folder, version, libcaesium_path, winsparkle_path,
                     imageformats_path):
    log('Run deploy for SETUP', 'warning')
    output_package_folder = os.path.join(output_folder, "caesium-image-compressor-" + version + "-win-setup")
    inno_compiler = "C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe"
    winsparkle_signer_path = "C:\\Program Files\\WinSparkle-0.7.0\\bin\\sign_update.bat"
    private_key_path = "C:\\Users\\matte\\Documents\\Progetti\\ssl\\dsa_priv.pem"

    log('Making output package folder...', 'info', " ")
    try:
        os.makedirs(output_package_folder)
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    executable_path = os.path.join(
        build_folder, "Caesium Image Compressor.exe")
    log('Deploying executable...', 'info', " ")
    deploy_result = subprocess.run(
        [windeployqt, executable_path, "--dir", output_package_folder, "--verbose=0"])

    try:
        deploy_result.check_returncode()
        shutil.copy(executable_path, os.path.join(output_package_folder, "Caesium Image Compressor.exe"))
        log('OK', 'success')
    except subprocess.CalledProcessError:
        log('FAIL', 'error')
        raise

    log('Copying additional libraries...', 'info', " ")
    try:
        shutil.copy(libcaesium_path, os.path.join(output_package_folder, "caesium.dll"))
        shutil.copy(winsparkle_path, os.path.join(output_package_folder, "WinSparkle.dll"))
        # shutil.copy(os.path.join(imageformats_path, "qicns.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qtga.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qwbmp.dll"), os.path.join(output_package_folder, "imageformats"))
        # shutil.copy(os.path.join(imageformats_path, "qwebp.dll"), os.path.join(output_package_folder, "imageformats"))
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Making installer...', 'info', " ")
    installer_result = subprocess.run(
        [inno_compiler, "/O" + output_folder, "/Q", os.path.join(source_folder, "setup.iss")])

    try:
        installer_result.check_returncode()
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Copying executable for web...', 'info', " ")

    try:
        shutil.copy(os.path.join(output_folder, "caesium-image-compressor-" + version + "-win-setup.exe"),
                    os.path.join(output_folder, 'com.saerasoft.caesium.exe'))
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Signing...', 'info', " ")
    sign_result = subprocess.run([winsparkle_signer_path, os.path.join(
        output_folder, 'com.saerasoft.caesium.exe'), private_key_path], capture_output=True, text=True)
    try:
        sign_result.check_returncode()
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    signature = sign_result.stdout.replace("\n", "")
    log('Generating appcast...', 'info', " ")
    try:
        size = os.path.getsize(os.path.join(output_folder, "com.saerasoft.caesium.exe"))
        xml = generate_appcast_win(version, signature, size)
        with open(os.path.join(output_folder, 'appcast.xml'), "w") as text_file:
            print(f"{xml}", file=text_file)
    except Exception:
        log('FAIL', 'error')
        raise

    log('OK', 'success')


def deploy_win():
    build_folder = sys.argv[1]
    build_folder_portable = sys.argv[2]
    version = sys.argv[3]
    output_folder = os.path.join(sys.argv[4], version)
    source_folder = sys.argv[5]
    windeployqt = sys.argv[6]
    libcaesium_path = os.path.join(build_folder,
                                   "libcaesium-prefix\\src\\libcaesium\\target\\x86_64-pc-windows-gnu\\release\\caesium.dll")
    winsparkle_path = os.path.join(build_folder,
                                   "libwinsparkle-prefix\\src\\libwinsparkle\\x64\\Release\\WinSparkle.dll")

    imageformats_path = "C:\\Qt\\6.5.2\\Src\\build-qtimageformats-Desktop_Qt_6_5_2_MinGW_64_bit-Release\\plugins\\imageformats"

    log('Deploying v' + version + ' into ' + output_folder, 'info')
    log('Clearing previous files...', 'info', " ")
    try:
        if os.path.exists(output_folder):
            shutil.rmtree(output_folder)
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    deploy_win_portable(windeployqt, build_folder_portable, output_folder, version, libcaesium_path, winsparkle_path,
                        imageformats_path)
    deploy_win_setup(windeployqt, source_folder, build_folder, output_folder, version, libcaesium_path,
                     winsparkle_path, imageformats_path)

    return 0


def generate_appcast_win(version, signature, size):
    time = datetime.now()
    xml_template = """<?xml version="1.0" standalone="yes"?>
    <rss xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle" version="2.0">
        <channel>
            <title>Caesium Image Compressor</title>
            <item>
                <title>%(version)s</title>
                <pubDate>%(date)s</pubDate>
                <enclosure
                    url="https://saerasoft.com/repository/com.saerasoft.caesium/win/release/com.saerasoft.caesium.exe"
                    sparkle:version="%(version)s"
                    length="%(size)i"
                    type="application/octet-stream"
                    sparkle:installerArguments="/SILENT /SUPPRESSMSGBOXES /SP- /NOICONS /CLOSEAPPLICATIONS"
                    sparkle:dsaSignature="%(signature)s"/>
            </item>
        </channel>
    </rss>
    """
    data = {'version': version, 'date': time.strftime("%a, %d %b %Y %H:%m:%S %z"), 'signature': signature, 'size': size}
    return xml_template % data


def deploy_osx():
    build_folder = sys.argv[1]
    version = sys.argv[2]
    output_folder = os.path.join(sys.argv[3], version)

    log('Deploying v' + version + ' into ' + output_folder, 'info')
    log('Clearing previous files...', 'info', " ")
    try:
        if os.path.exists(output_folder):
            shutil.rmtree(output_folder)
        os.makedirs(output_folder)
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    executable_path = os.path.join(
        build_folder, "Caesium Image Compressor.app")
    log('Making bundle...', 'info', " ")
    deploy_result = subprocess.run(
        ["macdeployqt", executable_path, "-always-overwrite", "-no-strip", "-dmg"])
    framework_copy_result = subprocess.run(['rsync', '-a', '--delete', '/Library/Frameworks/Sparkle.framework',
                                            os.path.join(executable_path, 'Contents', 'Frameworks')])
    try:
        deploy_result.check_returncode()
        framework_copy_result.check_returncode()
        log('OK', 'success')
    except subprocess.CalledProcessError:
        log('FAIL', 'error')
        raise

    log('Renaming DMG...', 'info', " ")
    try:
        dmg_name = 'caesium-image-compressor-' + version + '-macos.dmg'
        dmg_file_path = os.path.join(build_folder, dmg_name)
        dmg_destination = os.path.join(output_folder, dmg_name)
        try:
            os.remove(dmg_file_path)
        except FileNotFoundError:
            pass
        except Exception:
            raise
        os.rename(os.path.join(build_folder,
                               'Caesium Image Compressor.dmg'), dmg_file_path)
        shutil.move(dmg_file_path, dmg_destination)
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Compressing bundle...', 'info', " ")
    try:
        subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', executable_path,
                        os.path.join(output_folder, 'com.saerasoft.caesium.zip')])
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Signing...', 'info', " ")
    sign_result = subprocess.run(['sign_update', os.path.join(
        output_folder, 'com.saerasoft.caesium.zip')], capture_output=True, text=True)
    try:
        sign_result.check_returncode()
        log('OK', 'success')
    except Exception:
        log('FAIL', 'error')
        raise

    log('Generating appcast...', 'info', " ")
    xml = generate_appcast_osx(version, sign_result.stdout)
    try:
        with open(os.path.join(output_folder, 'appcast.xml'), "w") as text_file:
            print(f"{xml}", file=text_file)
    except Exception:
        log('FAIL', 'error')
        raise

    log('OK', 'success')
    return 0


def generate_appcast_osx(version, signature):
    time = datetime.now()
    xml_template = """<?xml version="1.0" standalone="yes"?>
    <rss xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle" version="2.0">
        <channel>
            <title>Caesium Image Compressor</title>
            <item>
                <title>%(version)s</title>
                <pubDate>%(date)s</pubDate>
                <sparkle:minimumSystemVersion>10.15</sparkle:minimumSystemVersion>
                <enclosure
                    url="https://saerasoft.com/repository/com.saerasoft.caesium/osx/release/com.saerasoft.caesium.zip"
                    sparkle:version="%(version)s"
                    sparkle:shortVersionString="%(version)s"
                    type="application/octet-stream"
                    %(signature)s />
            </item>
        </channel>
    </rss>"""
    data = {'version': version, 'date': time.strftime("%a, %d %b %Y %H:%m:%S %z"), 'signature': signature}
    return xml_template % data


if __name__ == '__main__':
    colorama.init()
    sys.exit(main())


