#!/usr/bin/env python
"""\
@file viewer_manifest.py
@author Ryan Williams
@brief Description of all installer viewer files, and methods for packaging
       them into installers for all supported platforms.

$LicenseInfo: firstyear=2006&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2006-2011, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import sys
import os.path
import re
import tarfile
import time
viewer_dir = os.path.dirname(__file__)
# add llmanifest library to our path so we don't have to muck with PYTHONPATH
sys.path.append(os.path.join(viewer_dir, '../lib/python/indra/util'))
from llmanifest import LLManifest, main, proper_windows_path, path_ancestors

class ViewerManifest(LLManifest):
    def is_packaging_viewer(self):
        # Some commands, files will only be included
        # if we are packaging the viewer on windows.
        # This manifest is also used to copy
        # files during the build (see copy_w_viewer_manifest
        # and copy_l_viewer_manifest targets)
        return 'package' in self.args['actions']
    
    def construct(self):
        super(ViewerManifest, self).construct()
        self.exclude("*.svn*")
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if self.is_packaging_viewer():
            if self.prefix(src="app_settings"):
                self.exclude("logcontrol.xml")
                self.exclude("logcontrol-dev.xml")
                self.path("*.pem")
                self.path("*.ini")
                self.path("*.xml")
                self.path("*.db2")

                # include the entire shaders directory recursively
                self.path("shaders")
                # ... and the entire windlight directory
                self.path("windlight")

                # ... and the included spell checking dictionaries
                pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
                if self.prefix(src=pkgdir,dst=""):
                    self.path("dictionaries")
                    self.end_prefix(pkgdir)

                self.end_prefix("app_settings")

            if self.prefix(src="character"):
                self.path("*.llm")
                self.path("*.xml")
                self.path("*.tga")
                self.end_prefix("character")

            # Include our fonts
            if self.prefix(src="fonts"):
                self.path("*.ttf")
                self.path("*.txt")
                self.end_prefix("fonts")

            # skins
            if self.prefix(src="skins"):
                    self.path("paths.xml")
                    # include the entire textures directory recursively
                    if self.prefix(src="*/textures"):
                            self.path("*/*.tga")
                            self.path("*/*.j2c")
                            self.path("*/*.jpg")
                            self.path("*/*.png")
                            self.path("*.tga")
                            self.path("*.j2c")
                            self.path("*.jpg")
                            self.path("*.png")
                            self.path("textures.xml")
                            self.end_prefix("*/textures")
                    self.path("*/xui/*/*.xml")
                    self.path("*/xui/*/widgets/*.xml")
                    self.path("*/*.xml")

                    # Local HTML files (e.g. loading screen)
                    if self.prefix(src="*/html"):
                            self.path("*.png")
                            self.path("*/*/*.html")
                            self.path("*/*/*.gif")
                            self.end_prefix("*/html")
                    self.end_prefix("skins")

            # local_assets dir (for pre-cached textures)
            if self.prefix(src="local_assets"):
                self.path("*.j2c")
                self.path("*.tga")
                self.end_prefix("local_assets")

            # Files in the newview/ directory
            self.path("gpu_table.txt")

            # The summary.json file gets left in the base checkout dir by
            # build.sh. It's only created for a build.sh build, therefore we
            # have to check whether it exists.  :-P
            summary_json = "summary.json"
            summary_json_path = os.path.join(os.pardir, os.pardir, summary_json)
            if os.path.exists(os.path.join(self.get_src_prefix(), summary_json_path)):
                self.path(summary_json_path, summary_json)
            else:
                print "No %s" % os.path.join(self.get_src_prefix(), summary_json_path)

    def login_channel(self):
        """Channel reported for login and upgrade purposes ONLY;
        used for A/B testing"""
        # NOTE: Do not return the normal channel if login_channel
        # is not specified, as some code may branch depending on
        # whether or not this is present
        return self.args.get('login_channel')

    def grid(self):
        return self.args['grid']
    def channel(self):
        return self.args['channel']
    def channel_unique(self):
        return self.channel().replace("Kokua", "").strip()
    def channel_oneword(self):
        return "".join(self.channel_unique().split())
    def channel_lowerword(self):
        return self.channel_oneword().lower()

    def icon_path(self):
        icon_path="icons/"
        channel_type=self.channel_lowerword()
        if channel_type == 'release' \
        or channel_type == 'development' \
        :
            icon_path += channel_type
        elif channel_type == 'betaviewer' :
            icon_path += 'beta'
        elif channel_type == 'kokua' :
            icon_path += 'kokua'
        elif re.match('project.*',channel_type) :
            icon_path += 'project'
        else :
            icon_path += 'kokua'
        return icon_path

    def flags_list(self):
        """ Convenience function that returns the command-line flags
        for the grid"""

        # Set command line flags relating to the target grid
        grid_flags = ''
        if not self.default_grid():
            grid_flags = "--grid %(grid)s "\
                         "--helperuri http://preview-%(grid)s.secondlife.com/helpers/" %\
                           {'grid':self.grid()}

        # set command line flags for channel
        channel_flags = ''
        if self.login_channel() and self.login_channel() != self.channel():
            # Report a special channel during login, but use default
            channel_flags = '--channel "%s"' % (self.login_channel())
        elif not self.default_channel():
            channel_flags = '--channel "%s"' % self.channel()

        # Deal with settings 
        setting_flags = ''
        if not self.default_channel() or not self.default_grid():
            if self.default_grid():
                setting_flags = '--settings settings_%s_%s.xml'\
                                % (self.channel_lowerword(), 'kokua')
            else:
                setting_flags = '--settings settings_%s_%s.xml'\
                                % (self.grid(), self.channel_lowerword())
                                                
        return " ".join((channel_flags, grid_flags, setting_flags)).strip()

class WindowsManifest(ViewerManifest):
    def final_exe(self):
        if self.default_channel():
            if self.default_grid():
                return "Kokua.exe"
            else:
                return "Kokua.exe"
        else:
            return ''.join(self.channel().split()) + '.exe'

    def test_msvcrt_and_copy_action(self, src, dst):
        # This is used to test a dll manifest.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                if(self.args['configuration'].lower() == 'debug'):
                    test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "8.0.50727.4053")
                else:
                    test_assembly_binding(src, "Microsoft.VC80.CRT", "8.0.50727.4053")
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src

    def test_for_no_msvcrt_manifest_and_copy_action(self, src, dst):
        # This is used to test that no manifest for the msvcrt exists.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        from test_win32_manifest import NoManifestException, NoMatchingAssemblyException
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                try:
                    if(self.args['configuration'].lower() == 'debug'):
                        test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "")
                    else:
                        test_assembly_binding(src, "Microsoft.VC80.CRT", "")
                    raise Exception("Unknown condition")
                except NoManifestException, err:
                    pass
                except NoMatchingAssemblyException, err:
                    pass
                    
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src
        
    ### DISABLED MANIFEST CHECKING for vs2010.  we may need to reenable this
    # shortly.  If this hasn't been reenabled by the 2.9 viewer release then it
    # should be deleted -brad
    #def enable_crt_manifest_check(self):
    #    if self.is_packaging_viewer():
    #       WindowsManifest.copy_action = WindowsManifest.test_msvcrt_and_copy_action

    #def enable_no_crt_manifest_check(self):
    #    if self.is_packaging_viewer():
    #        WindowsManifest.copy_action = WindowsManifest.test_for_no_msvcrt_manifest_and_copy_action

    #def disable_manifest_check(self):
    #    if self.is_packaging_viewer():
    #        del WindowsManifest.copy_action

    def construct(self):
        super(WindowsManifest, self).construct()

        #self.enable_crt_manifest_check()

        if self.is_packaging_viewer():
            # Find kokua-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
            self.path(src='%s/kokua-bin.exe' % self.args['configuration'], dst=self.final_exe())

        # Plugin host application
        self.path(os.path.join(os.pardir,
                               'llplugin', 'slplugin', self.args['configuration'], "slplugin.exe"),
                  "slplugin.exe")
        
        #self.disable_manifest_check()

        self.path(src="../viewer_components/updater/scripts/windows/update_install.bat", dst="update_install.bat")
        # Get shared libs from the shared libs staging directory
        if self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            #self.enable_crt_manifest_check()
            
            # Get llcommon and deps. If missing assume static linkage and continue.
            try:
                self.path('llcommon.dll')
                self.path('libapr-1.dll')
                self.path('libaprutil-1.dll')
                self.path('libapriconv-1.dll')
                
            except RuntimeError, err:
                print err.message
                print "Skipping llcommon.dll (assuming llcommon was linked statically)"

            #self.disable_manifest_check()

            # Mesh 3rd party libs needed for auto LOD and collada reading
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path("libcollada14dom22-d.dll")
                else:
                    self.path("libcollada14dom22.dll")
                    
                self.path("glod.dll")
            except RuntimeError, err:
                print err.message
                print "Skipping COLLADA and GLOD libraries (assumming linked statically)"


            # Get fmod dll, continue if missing
            try:
                self.path("fmod.dll")
            except:
                print "Skipping fmod.dll"

            # For textures
            if self.args['configuration'].lower() == 'debug':
                self.path("openjpegd.dll")
            else:
                self.path("openjpeg.dll")

            # These need to be installed as a SxS assembly, currently a 'private' assembly.
            # See http://msdn.microsoft.com/en-us/library/ms235291(VS.80).aspx
            if self.args['configuration'].lower() == 'debug':
                 self.path("msvcr100d.dll")
                 self.path("msvcp100d.dll")
            else:
                 self.path("msvcr100.dll")
                 self.path("msvcp100.dll")

            # Vivox runtimes
#            self.path("wrap_oal.dll") no longer in archive
            self.path("SLVoice.exe")
            self.path("vivoxsdk.dll")
            self.path("ortp.dll")
#           added from archive
            self.path("libsndfile-1.dll")
            self.path("vivoxoal.dll")
            self.path("vivoxplatform.dll")
            try:
                self.path("zlib1.dll")
            except:
                print "Skipping zlib1.dll"

				# Security
            self.path("ssleay32.dll")
            self.path("libeay32.dll")				

            # Hunspell
            self.path("libhunspell.dll")

        #OpenAL
        try:
            self.path("openal32.dll")
            self.path("alut.dll")
        except:
            print "Skipping openal"
        # Gstreamer libs
        try:
            self.path("avcodec-gpl-52.dll")
            self.path("avdevice-gpl-52.dll")
            self.path("avfilter-gpl-1.dll")
            self.path("avformat-gpl-52.dll")
            self.path("avutil-gpl-50.dll")
            self.path("iconv.dll")
            self.path("liba52-0.dll")
            self.path("libbz2.dll")
            self.path("libcelt-0.dll")
            self.path("libdca-0.dll")
            self.path("libexpat-1.dll")
            self.path("libfaad-2.dll")
            self.path("libFLAC-8.dll")
            self.path("libgcrypt-11.dll")
            self.path("libgio-2.0-0.dll")
            self.path("libglib-2.0-0.dll")
            self.path("libgmodule-2.0-0.dll")
            self.path("libgnutls-26.dll")
            self.path("libgobject-2.0-0.dll")
            self.path("libgpg-error-0.dll")
            self.path("libgstapp-0.10.dll")
            self.path("libgstaudio-0.10.dll")
            self.path("libgstbase-0.10.dll")
            self.path("libgstcontroller-0.10.dll")
            self.path("libgstdataprotocol-0.10.dll")
            self.path("libgstfft-0.10.dll")
            self.path("libgstinterfaces-0.10.dll")
            self.path("libgstnet-0.10.dll")
            self.path("libgstnetbuffer-0.10.dll")
            self.path("libgstpbutils-0.10.dll")
            self.path("libgstphotography-0.10.dll")
            self.path("libgstreamer-0.10.dll")
            self.path("libgstriff-0.10.dll")
            self.path("libgstrtp-0.10.dll")
            self.path("libgstrtsp-0.10.dll")
            self.path("libgstsdp-0.10.dll")
            self.path("libgstsignalprocessor-0.10.dll")
            self.path("libgsttag-0.10.dll")
            self.path("libgstvideo-0.10.dll")
            self.path("libgthread-2.0-0.dll")
            self.path("libmms-0.dll")
            self.path("libmpeg2-0.dll")
            self.path("libneon-27.dll")
            self.path("libogg-0.dll")
            self.path("liboil-0.3-0.dll")
            self.path("libsoup-2.4-1.dll")
            self.path("libtasn1-3.dll")
            self.path("libtheora-0.dll")
            self.path("libtheoradec-1.dll")
            self.path("libvorbis-0.dll")
            self.path("libvorbisenc-2.dll")
            self.path("libvorbisfile-3.dll")
            self.path("libwavpack-1.dll")
            self.path("libx264-67.dll")
            self.path("libxml2-2.dll")
            self.path("libxml2.dll")
            self.path("SDL.dll")
            self.path("xvidcore.dll")
            self.path("z.dll")
        except:
            print "Skipping gstreamer libraries"

			
		# For google-perftools tcmalloc allocator.
	try:
		if self.args['configuration'].lower() == 'debug':
			self.path('libtcmalloc_minimal-debug.dll')
		else:
			self.path('libtcmalloc_minimal.dll')
	except:
			print "Skipping libtcmalloc_minimal.dll"
			
			self.path(src="licenses-win32.txt", dst="licenses.txt")
			self.path("featuretable.txt")
			self.path("featuretable_xp.txt")

        self.end_prefix()

	self.path(src="licenses-win32.txt", dst="licenses.txt")
	self.path("featuretable.txt")
	self.path("featuretable_xp.txt")
    
        #self.enable_no_crt_manifest_check()

        # On first build tries to copy before it is built.
        if self.prefix(src='../media_plugins/gstreamer010/%s' % self.args['configuration'], dst="llplugin"):
            try:
                self.path("media_plugin_gstreamer010.dll")
            except:
                print "Skipping media_plugin_gstreamer010.dll" 
            self.end_prefix()

        # Gstreamer plugins
        if self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release', 'gstreamer-plugins'),
            dst="llplugin/gstreamer-plugins"):
            try:
               #self.path("*.dll") #why does this nothing?
               self.path("libgsta52dec.dll")
               self.path("libgstadder.dll")
               self.path("libgstapetag.dll")
               self.path("libgstapp.dll")
               self.path("libgstasf.dll")
               self.path("libgstasfmux.dll")
               self.path("libgstaudioconvert.dll")
               self.path("libgstaudiofx.dll")
               self.path("libgstaudiorate.dll")
               self.path("libgstaudioresample.dll")
               self.path("libgstauparse.dll")
               self.path("libgstautoconvert.dll")
               self.path("libgstautodetect.dll")
               self.path("libgstbz2.dll")
               self.path("libgstcelt.dll")
               self.path("libgstcoreelements.dll")
               self.path("libgstdecodebin.dll")
               self.path("libgstdecodebin2.dll")
               self.path("libgstdirectsound.dll")
               self.path("libgstdirectsoundsrc.dll")
               self.path("libgstdtsdec.dll")
               self.path("libgstequalizer.dll")
               self.path("libgstfaad.dll")
               self.path("libgstffmpeg-gpl.dll")
               self.path("libgstflac.dll")
               self.path("libgstfreeze.dll")
               self.path("libgstgdp.dll")
               self.path("libgstgio.dll")
               self.path("libgsth264parse.dll")
               self.path("libgsticydemux.dll")
               self.path("libgstid3demux.dll")
               self.path("libgstinterleave.dll")
               self.path("libgstlegacyresample.dll")
               self.path("libgstlevel.dll")
               self.path("libgstliveadder.dll")
               self.path("libgstmms.dll")
               self.path("libgstmpeg2dec.dll")
               self.path("libgstmpegaudioparse.dll")
               self.path("libgstmpegdemux.dll")
               self.path("libgstmpegpsmux.dll")
               self.path("libgstmpegstream.dll")
               self.path("libgstmpegtsmux.dll")
               self.path("libgstneonhttpsrc.dll")
               self.path("libgstogg.dll")
               self.path("libgstplaybin.dll")
               self.path("libgstqtdemux.dll")
               self.path("libgstqtmux.dll")
               self.path("libgstrawparse.dll")
               self.path("libgstreal.dll")
               self.path("libgstrtp.dll")
               self.path("libgstrtpdemux.dll")
               self.path("libgstrtpjitterbuffer.dll")
               self.path("libgstrtpmanager.dll")
               self.path("libgstrtpmux.dll")
               self.path("libgstrtppayloads.dll")
               self.path("libgstrtsp.dll")
               self.path("libgstscaletempoplugin.dll")
               self.path("libgstsdl.dll")
               self.path("libgstsdpelem.dll")
               self.path("libgstsouphttpsrc.dll")
               self.path("libgststereo.dll")
               self.path("libgsttta.dll")
               self.path("libgsttypefindfunctions.dll")
               self.path("libgstudp.dll")
               self.path("libgstvalve.dll")
               self.path("libgstvolume.dll")
               self.path("libgstvorbis.dll")
               self.path("libgstwasapi.dll")
               self.path("libgstwaveformsink.dll")
               self.path("libgstwavpack.dll")
               self.path("libgstwavparse.dll")
               self.path("libgstwininet.dll")
               self.path("libgstx264.dll")
            except:
                print "Skipping gstreamer-plugins"
        self.end_prefix()

        # Media plugins - QuickTime
        if self.prefix(src='../media_plugins/quicktime/%s' % self.args['configuration'], dst="llplugin"):
           try:
               self.path("media_plugin_quicktime.dll")
           except:
               print "Skipping media_plugin_quicktime.dll"
           self.end_prefix()

        # Media plugins - WebKit/Qt
        if self.prefix(src='../media_plugins/webkit/%s' % self.args['configuration'], dst="llplugin"):
            try:
                self.path("media_plugin_webkit.dll")
            except:
                print "Skipping media_plugin_webkit.dll"
            self.end_prefix()

        # winmm.dll shim
        if self.prefix(src='../media_plugins/winmmshim/%s' % self.args['configuration'], dst=""):
            try:
                self.path("winmm.dll")
            except:
                print "Skipping winmm.dll"
            self.end_prefix()


        if self.args['configuration'].lower() == 'debug':
            if self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'debug'),
                           dst="llplugin"):
                self.path("libeay32.dll")
                self.path("qtcored4.dll")
                self.path("qtguid4.dll")
                self.path("qtnetworkd4.dll")
                self.path("qtopengld4.dll")
                self.path("qtwebkitd4.dll")
                self.path("qtxmlpatternsd4.dll")
                self.path("ssleay32.dll")

                # For WebKit/Qt plugin runtimes (image format plugins)
                if self.prefix(src="imageformats", dst="imageformats"):
                    self.path("qgifd4.dll")
                    self.path("qicod4.dll")
                    self.path("qjpegd4.dll")
                    self.path("qmngd4.dll")
                    self.path("qsvgd4.dll")
                    self.path("qtiffd4.dll")
                    self.end_prefix()

                # For WebKit/Qt plugin runtimes (codec/character encoding plugins)
                if self.prefix(src="codecs", dst="codecs"):
                    self.path("qcncodecsd4.dll")
                    self.path("qjpcodecsd4.dll")
                    self.path("qkrcodecsd4.dll")
                    self.path("qtwcodecsd4.dll")
                    self.end_prefix()

                self.end_prefix()
        else:
            if self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release'),
                           dst="llplugin"):
                self.path("libeay32.dll")
                self.path("qtcore4.dll")
                self.path("qtgui4.dll")
                self.path("qtnetwork4.dll")
                self.path("qtopengl4.dll")
                self.path("qtwebkit4.dll")
                self.path("qtxmlpatterns4.dll")
                self.path("ssleay32.dll")

                # For WebKit/Qt plugin runtimes (image format plugins)
                if self.prefix(src="imageformats", dst="imageformats"):
                    self.path("qgif4.dll")
                    self.path("qico4.dll")
                    self.path("qjpeg4.dll")
                    self.path("qmng4.dll")
                    self.path("qsvg4.dll")
                    self.path("qtiff4.dll")
                    self.end_prefix()

                # For WebKit/Qt plugin runtimes (codec/character encoding plugins)
                if self.prefix(src="codecs", dst="codecs"):
                    self.path("qcncodecs4.dll")
                    self.path("qjpcodecs4.dll")
                    self.path("qkrcodecs4.dll")
                    self.path("qtwcodecs4.dll")
                    self.end_prefix()

                self.end_prefix()

        #self.disable_manifest_check()

        # pull in the crash logger and updater from other projects
        # tag:"crash-logger" here as a cue to the exporter
        self.path(src='../win_crash_logger/%s/windows-crash-logger.exe' % self.args['configuration'],
                  dst="win_crash_logger.exe")
        self.path(src='../win_updater/%s/windows-updater.exe' % self.args['configuration'],
                  dst="updater.exe")

        if not self.is_packaging_viewer():
            self.package_file = "copied_deps"    

    def nsi_file_commands(self, install=True):
        def wpath(path):
            if path.endswith('/') or path.endswith(os.path.sep):
                path = path[:-1]
            path = path.replace('/', '\\')
            return path

        result = ""
        dest_files = [pair[1] for pair in self.file_list if pair[0] and os.path.isfile(pair[1])]
        # sort deepest hierarchy first
        dest_files.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
        dest_files.reverse()
        out_path = None
        for pkg_file in dest_files:
            rel_file = os.path.normpath(pkg_file.replace(self.get_dst_prefix()+os.path.sep,''))
            installed_dir = wpath(os.path.join('$INSTDIR', os.path.dirname(rel_file)))
            pkg_file = wpath(os.path.normpath(pkg_file))
            if installed_dir != out_path:
                if install:
                    out_path = installed_dir
                    result += 'SetOutPath ' + out_path + '\n'
            if install:
                result += 'File ' + pkg_file + '\n'
            else:
                result += 'Delete ' + wpath(os.path.join('$INSTDIR', rel_file)) + '\n'

        # at the end of a delete, just rmdir all the directories
        if not install:
            deleted_file_dirs = [os.path.dirname(pair[1].replace(self.get_dst_prefix()+os.path.sep,'')) for pair in self.file_list]
            # find all ancestors so that we don't skip any dirs that happened to have no non-dir children
            deleted_dirs = []
            for d in deleted_file_dirs:
                deleted_dirs.extend(path_ancestors(d))
            # sort deepest hierarchy first
            deleted_dirs.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
            deleted_dirs.reverse()
            prev = None
            for d in deleted_dirs:
                if d != prev:   # skip duplicates
                    result += 'RMDir ' + wpath(os.path.join('$INSTDIR', os.path.normpath(d))) + '\n'
                prev = d

        return result

    def package_finish(self):
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'version' : '.'.join(self.args['version']),
            'version_short' : '.'.join(self.args['version'][:-1]),
            'version_dashes' : '-'.join(self.args['version']),
            'final_exe' : self.final_exe(),
            'grid':self.args['grid'],
            'grid_caps':self.args['grid'].upper(),
            # escape quotes becase NSIS doesn't handle them well
            'flags':self.flags_list().replace('"', '$\\"'),
            'channel':self.channel(),
            'channel_oneword':self.channel_oneword(),
            'channel_unique':self.channel_unique(),
            }

        version_vars = """
        !define INSTEXE  "%(final_exe)s"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        """ % substitution_strings
        if self.default_channel():
            if self.default_grid():
                # release viewer
                installer_file = "Kokua-%(version_dashes)s_Setup.exe"
                grid_vars_template = """
                OutFile "%(installer_file)s"
                !define INSTFLAGS "%(flags)s"
                !define INSTNAME   "Kokua"
                !define SHORTCUT   "Kokua"
                !define URLNAME   "kokua"
                Caption "Kokua ${VERSION}"
                """
            else:
                # beta grid viewer
                installer_file = "Kokua-%(version_dashes)s_(%(grid_caps)s)_Setup.exe"
                grid_vars_template = """
                OutFile "%(installer_file)s"
                !define INSTFLAGS "%(flags)s"
                !define INSTNAME   "Kokua%(grid_caps)s"
                !define SHORTCUT   "Kokua (%(grid_caps)s)"
                !define URLNAME   "kokua%(grid)s"
                !define UNINSTALL_SETTINGS 1
                Caption "Kokua %(grid)s ${VERSION}"
                """
        else:
            # some other channel on some grid
            installer_file = "Kokua_-%(version_dashes)s_%(channel_oneword)s_Setup.exe"
            grid_vars_template = """
            OutFile "%(installer_file)s"
            !define INSTFLAGS "%(flags)s"
            !define INSTNAME   "Kokua%(channel_oneword)s"
            !define SHORTCUT   "%(channel)s"
            !define URLNAME   "kokua"
            !define UNINSTALL_SETTINGS 1
            Caption "%(channel)s ${VERSION}"
            """
        if 'installer_name' in self.args:
            installer_file = self.args['installer_name']
        else:
            installer_file = installer_file % substitution_strings
        substitution_strings['installer_file'] = installer_file

        tempfile = "kokua_setup_tmp.nsi"
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%GRID_VARS%%":grid_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%DELETE_FILES%%":self.nsi_file_commands(False)})

        # We use the Unicode version of NSIS, available from
        # http://www.scratchpaper.com/
        # Check two paths, one for Program Files, and one for Program Files (x86).
        # Yay 64bit windows.
        NSIS_path = os.path.expandvars('${ProgramFiles}\\NSIS\\Unicode\\makensis.exe')
        if not os.path.exists(NSIS_path):
            NSIS_path = os.path.expandvars('${ProgramFiles(x86)}\\NSIS\\Unicode\\makensis.exe')
        self.run_command('"' + proper_windows_path(NSIS_path) + '" ' + self.dst_path_of(tempfile))
        # self.remove(self.dst_path_of(tempfile))
        # If we're on a build machine, sign the code using our Authenticode certificate. JC
        sign_py = os.path.expandvars("${SIGN}")
        if not sign_py or sign_py == "${SIGN}":
            sign_py = 'C:\\buildscripts\\code-signing\\sign.py'
        else:
            sign_py = sign_py.replace('\\', '\\\\\\\\')
        python = os.path.expandvars("${PYTHON}")
        if not python or python == "${PYTHON}":
            python = 'python'
        if os.path.exists(sign_py):
            self.run_command("%s %s %s" % (python, sign_py, self.dst_path_of(installer_file).replace('\\', '\\\\\\\\')))
        else:
            print "Skipping code signing,", sign_py, "does not exist"
        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file


class DarwinManifest(ViewerManifest):
    def is_packaging_viewer(self):
        # darwin requires full app bundle packaging even for debugging.
        return True

    def construct(self):
        # copy over the build result (this is a no-op if run within the xcode script)
        self.path(self.args['configuration'] + "/Kokua.app", dst="")

        if self.prefix(src="", dst="Contents"):  # everything goes in Contents

            # copy additional libs in <bundle>/Contents/MacOS/

            if self.prefix(src="../../libraries/universal-darwin/lib_release/", dst="MacOS"):
                self.path("libndofdev.dylib")
                self.path("libalut.0.dylib")
                self.path("libopenal.1.dylib")
                self.path("libopenjpeg.1.4.dylib")
                self.end_prefix("MacOS")

            self.path("../packages/lib/release/libndofdev.dylib", dst="Resources/libndofdev.dylib")
            self.path("../packages/lib/release/libhunspell-1.3.0.dylib", dst="Resources/libhunspell-1.3.0.dylib")

	    self.path("../viewer_components/updater/scripts/darwin/update_install", "MacOS/update_install")

            # Info.plist goes directly in Contents
            self.path("packaging/mac/Info.plist", dst="Info.plist")

            # most everything goes in the Resources directory
            if self.prefix(src="", dst="Resources"):
                super(DarwinManifest, self).construct()

                if self.prefix("cursors_mac"):
                    self.path("*.tif")
                    self.end_prefix("cursors_mac")

                self.path("licenses-mac.txt", dst="licenses.txt")
                self.path("featuretable_mac.txt")

                self.path("viewer.icns")

                if self.prefix(src="packaging/mac", dst=""):
                    self.path("Kokua.nib")

                    # Translations
                    self.path("English.lproj")
                    self.path("German.lproj")
                    self.path("Japanese.lproj")
                    self.path("Korean.lproj")
                    self.path("da.lproj")
                    self.path("es.lproj")
                    self.path("fr.lproj")
                    self.path("hu.lproj")
                    self.path("it.lproj")
                    self.path("nl.lproj")
                    self.path("pl.lproj")
                    self.path("pt.lproj")
                    self.path("ru.lproj")
                    self.path("tr.lproj")
                    self.path("uk.lproj")
                    self.path("zh-Hans.lproj")
                    self.end_prefix("packaging/mac")

                # SLVoice and vivox lols

                self.path("vivox-runtime/universal-darwin/libalut.dylib", "libalut.dylib")
                self.path("vivox-runtime/universal-darwin/libopenal.dylib", "libopenal.dylib")
                self.path("vivox-runtime/universal-darwin/libortp.dylib", "libortp.dylib")
                self.path("vivox-runtime/universal-darwin/libvivoxsdk.dylib", "libvivoxsdk.dylib")
                self.path("vivox-runtime/universal-darwin/SLVoice", "SLVoice")

                libdir = "../../libraries/universal-darwin/lib_release"
                libdir = "../packages/lib/release"
                dylibs = {}

                # Need to get the llcommon dll from any of the build directories as well
                lib = "llcommon"
                libfile = "lib%s.dylib" % lib
                try:
                    self.path(self.find_existing_file(os.path.join(os.pardir,
                                                                    lib,
                                                                    self.args['configuration'],
                                                                    libfile),
                                                      os.path.join(libdir, libfile)),
                                                      dst=libfile)
                except RuntimeError:
                    print "Skipping %s" % libfile
                    dylibs[lib] = False
                else:
                    dylibs[lib] = True

                if dylibs["llcommon"]:
                    for libfile in ("libapr-1.0.dylib",
                                    "libaprutil-1.0.dylib",
                                    "libexpat.1.5.2.dylib",
                                    "libexception_handler.dylib",
                                    "libGLOD.dylib",
                                    "libcollada14dom.dylib"
                                    ):
                        self.path(os.path.join(libdir, libfile), libfile)

                # SLVoice and vivox lols
                for libfile in ('libsndfile.dylib', 'libvivoxoal.dylib', 'libortp.dylib', \
                    'libvivoxsdk.dylib', 'libvivoxplatform.dylib', 'SLVoice') :
                     self.path(os.path.join(libdir, libfile), libfile)
                
                try:
                    # FMOD for sound
                    self.path(self.args['configuration'] + "/libfmodwrapper.dylib", "libfmodwrapper.dylib")
                except:
                    print "Skipping FMOD - not found"
                
                # our apps
                self.path("../mac_crash_logger/" + self.args['configuration'] + "/mac-crash-logger.app", "mac-crash-logger.app")
                self.path("../mac_updater/" + self.args['configuration'] + "/mac-updater.app", "mac-updater.app")

                # plugin launcher
                self.path("../llplugin/slplugin/" + self.args['configuration'] + "/SLPlugin.app", "SLPlugin.app")

                # our apps dependencies on shared libs
                if dylibs["llcommon"]:
                    mac_crash_logger_res_path = self.dst_path_of("mac-crash-logger.app/Contents/Resources")
                    mac_updater_res_path = self.dst_path_of("mac-updater.app/Contents/Resources")
                    slplugin_res_path = self.dst_path_of("SLPlugin.app/Contents/Resources")
                    for libfile in ("libllcommon.dylib",
                                    "libapr-1.0.dylib",
                                    "libaprutil-1.0.dylib",
                                    "libexpat.1.5.2.dylib",
                                    "libexception_handler.dylib",
                                    "libGLOD.dylib",
                                    "libcollada14dom.dylib"
                                    ):
                        target_lib = os.path.join('../../..', libfile)
                        self.run_command("ln -sf %(target)r %(link)r" % 
                                         {'target': target_lib,
                                          'link' : os.path.join(mac_crash_logger_res_path, libfile)}
                                         )
                        self.run_command("ln -sf %(target)r %(link)r" % 
                                         {'target': target_lib,
                                          'link' : os.path.join(mac_updater_res_path, libfile)}
                                         )
                        self.run_command("ln -sf %(target)r %(link)r" % 
                                         {'target': target_lib,
                                          'link' : os.path.join(slplugin_res_path, libfile)}
                                         )

                # plugins
                if self.prefix(src="", dst="llplugin"):
                    self.path("../media_plugins/webkit/" + self.args['configuration'] + "/media_plugin_webkit.dylib", "media_plugin_webkit.dylib")
                    self.path("../packages/lib/release/libllqtwebkit.dylib", "libllqtwebkit.dylib")
                    self.end_prefix("llplugin")

                # command line arguments for connecting to the proper grid
                self.put_in_file(self.flags_list(), 'arguments.txt')

                self.end_prefix("Resources")

            self.end_prefix("Contents")

        # NOTE: the -S argument to strip causes it to keep enough info for
        # annotated backtraces (i.e. function names in the crash log).  'strip' with no
        # arguments yields a slightly smaller binary but makes crash logs mostly useless.
        # This may be desirable for the final release.  Or not.
        if ("package" in self.args['actions'] or 
            "unpacked" in self.args['actions']):
            self.run_command('strip -S %(viewer_binary)r' %
                             { 'viewer_binary' : self.dst_path_of('Contents/MacOS/Kokua')})



    def copy_finish(self):
        # Force executable permissions to be set for scripts
        # see CHOP-223 and http://mercurial.selenic.com/bts/issue1802
        for script in 'Contents/MacOS/update_install',:
            self.run_command("chmod +x %r" % os.path.join(self.get_dst_prefix(), script))


class LinuxManifest(ViewerManifest):
    def construct(self):
        super(LinuxManifest, self).construct()
        self.path("licenses-linux.txt","licenses.txt")
        self.path("res/kokua_icon.png", "kokua_icon.png")
        if self.prefix("linux_tools", dst=""):
            self.path("client-readme.txt","README-linux.txt")
            self.path("client-readme-voice.txt","README-linux-voice.txt")
            self.path("client-readme-joystick.txt","README-linux-joystick.txt")
            self.path("wrapper.sh","kokua")
            self.path("handle_secondlifeprotocol.sh", "etc/handle_secondlifeprotocol.sh")
            self.path("register_secondlifeprotocol.sh", "etc/register_secondlifeprotocol.sh")
            self.path("register_hopprotocol.sh", "etc/register_hopprotocol.sh")
            self.path("refresh_desktop_app_entry.sh", "etc/refresh_desktop_app_entry.sh")
            self.path("launch_url.sh","etc/launch_url.sh")
            self.path("install.sh")
            self.end_prefix("linux_tools")

        # Create an appropriate gridargs.dat for this package, denoting required grid.
        self.put_in_file(self.flags_list(), 'etc/gridargs.dat')

        self.path("kokua-bin","bin/do-not-directly-run-kokua-bin")
        self.path("../linux_crash_logger/linux-crash-logger","bin/linux-crash-logger.bin")
        self.path("../linux_updater/linux-updater", "bin/linux-updater.bin")
        self.path("../llplugin/slplugin/SLPlugin", "bin/SLPlugin")

        if self.prefix("res-sdl"):
            self.path("*")
            # recurse
            self.end_prefix("res-sdl")

        # Get the icons based on the channel
        icon_path = self.icon_path()
        if self.prefix(src=icon_path, dst="") :
            self.path("kokua_icon.png")
            if self.prefix(src="",dst="res-sdl") :
                self.path("kokua_icon.BMP")
                self.end_prefix("res-sdl")
            self.end_prefix(icon_path)

        self.path("../viewer_components/updater/scripts/linux/update_install", "bin/update_install")

        # plugins
        if self.prefix(src="", dst="bin/llplugin"):
            self.path("../media_plugins/webkit/libmedia_plugin_webkit.so", "libmedia_plugin_webkit.so")
            self.path("../media_plugins/gstreamer010/libmedia_plugin_gstreamer010.so", "libmedia_plugin_gstreamer.so")
            self.end_prefix("bin/llplugin")



        self.path("featuretable_linux.txt")
        self.package_file = "foo"
    def copy_finish(self):
        # Force executable permissions to be set for scripts
        # see CHOP-223 and http://mercurial.selenic.com/bts/issue1802

        for script in ('install.sh', 'kokua', 'bin/update_install'):
            self.run_command("chmod +x %r" % os.path.join(self.get_dst_prefix(), script))
        #yus, copy paste was faster :P
#        for script in ('install.sh', 'kokua', 'bin/update_install', 'etc/handle_secondlifeprotocol.sh',
#                       'etc/register_secondlifeprotocol.sh', 'etc/register_hopprotocol.sh',
#                       'etc/refresh_desktop_app_entry.sh', 'etc/launch_url.sh'):
#                           self.run_command("chmod +x %r" % os.path.join(self.get_dst_prefix(), script))

class Linux_i686Manifest(LinuxManifest):
    def construct(self):
        super(Linux_i686Manifest, self).construct()


        # install either the libllkdu we just built, or a prebuilt one, in
        # decreasing order of preference.  for linux package, this goes to bin/
        try:
            self.path(self.find_existing_file('../llkdu/libllkdu.so',
                '../../libraries/i686-linux/lib_release_client/libllkdu.so'),
                  dst='bin/libllkdu.so')
        except:
            print "Skipping libllkdu.so - not found"

        try:
            self.path("../llcommon/libllcommon.so", "lib/libllcommon.so")
        except:
            print "Skipping llcommon.so (assuming llcommon was linked statically)"


        if self.prefix("../packages/lib/release", dst="lib"):
            self.path("libapr-1.so*")
            self.path("libaprutil-1.so*")
            self.path("libbreakpad_client.so*")
            self.path("libcollada14dom.so")
            self.path("libdb*.so")
            self.path("libcrypto.so.*")
            self.path("libexpat.so.*")
            self.path("libssl.so")
            self.path("libssl.so.1.0.0")
            self.path("libglod.so")
            self.path("libminizip.so")
            self.path("libuuid.so*")
            self.path("libSDL-1.2.so.*")
            self.path("libdirectfb-1.*.so.*")
            self.path("libfusion-1.*.so.*")
            self.path("libdirect-1.*.so.*")
            self.path("libopenjpeg.so*")
            self.path("libdirectfb-1.4.so.5")
            self.path("libfusion-1.4.so.5")
            self.path("libdirect-1.4.so.5.0.4")
            self.path("libdirect-1.4.so.5")
            self.path("libhunspell-1.3.so")
            self.path("libhunspell-1.3.so.0")
            self.path("libhunspell-1.3.so.0.0.0")
            self.path("libalut.so")
            self.path("libalut.so.0")
            self.path("libalut.so.0.0.0")
            self.path("libopenal.so")
            self.path("libopenal.so.1")
            self.path("libopenal.so.1.13.0")
            self.path("libopenal.so", "libvivoxoal.so.1") # vivox's sdk expects this soname
            self.path("libfontconfig.so*")
            self.path("libpng15.so.15") 
            self.path("libpng15.so.15.10.0")            
            try:
                self.path("libtcmalloc.so*") #formerly called google perf tools
                pass
            except:
                print "tcmalloc files not found, skipping"
                pass

            self.end_prefix("lib")

            # Vivox runtimes
            if self.prefix(src="../packages/lib/release/", dst="bin"):
                    self.path("SLVoice")
                    self.end_prefix()
            if self.prefix(src="../packages/lib/release/", dst="lib"):
                    self.path("libortp.so")
                    self.path("libsndfile.so.1")
                    self.path("libvivoxsdk.so")
                    self.path("libvivoxplatform.so")
                    self.end_prefix("lib")

        if self.args['buildtype'].lower() == 'release':
                self.run_command('find %(d)r/bin %(d)r/lib  -type f \\'
                                 '! -name update_install | xargs --no-run-if-empty strip -S'
                                 % {'d': self.get_dst_prefix()} )


class Linux_x86_64Manifest(LinuxManifest):
    def construct(self):
        super(Linux_x86_64Manifest, self).construct()

        # support file for valgrind debug tool
        self.path("secondlife-i686.supp")

	try:
            self.path("../llcommon/libllcommon.so", "lib64/libllcommon.so")
        except:
            print "Skipping llcommon.so (assuming llcommon was linked statically)"

        if self.prefix("../packages/lib/release", dst="lib64"):
            self.path("libapr-1.so*")
            self.path("libaprutil-1.so*")
            self.path("libbreakpad_client.so*")
            self.path("libdb-5.1.so")
            self.path("libdb-5.so")
            self.path("libdb.so")
            self.path("libcares.so.2.0.0", "libcares.so.2")
            self.path("libcurl.so.4.2.0", "libcurl.so.4")
            self.path("libcrypto.so.1.0.0")
            self.path("libssl.so")
            self.path("libssl.so.1.0.0")
            self.path("libexpat.so.1")
            self.path("libSDL-1.2.so.0.11.3","libSDL-1.2.so.0")
            self.path("libjpeg.so")
            self.path("libjpeg.so.8")
            self.path("libjpeg.so.8.3.0")
            self.path("libopenjpeg.so.1.4.0")
            self.path("libopenjpeg.so.1")
            self.path("libopenjpeg.so")
            self.path("libhunspell-1.3.so")
            self.path("libhunspell-1.3.so.0")
            self.path("libhunspell-1.3.so.0.0.0")
            self.path("libpcre.so")
            self.path("libpcre.so.3")
            self.path("libpcrecpp.so")
            self.path("libpcrecpp.so.0")
            self.path("libminizip.so")
            self.path("libminizip.so.1")
            self.path("libminizip.so.1.2.3")
            self.path("libxml2.so.2.7.8")
            self.path("libz.so.1.2.5")
            self.path("libz.so.1")
            self.path("libz.so")
            # KOKUA Library self.path("libcollada14dom.so.2.3.0","libcollada14dom.so.2")
			self.path("libcollada14dom.so.2.2")
            self.path("libcollada14dom.so.2")
            self.path("libcollada14dom.so")
            self.path("libglod.so")

            # OpenAL
            self.path("libalut.so")
            self.path("libalut.so.0")
            self.path("libopenal.so")
            self.path("libopenal.so.1")
            self.path("libalut.so.0.1.0")
            self.path("libopenal.so.1.12.854")
            self.path("libfontconfig.so*")
            self.path("libpng15.so.15") 
            self.path("libpng15.so.15.10.0")
            self.end_prefix("lib64")

            # Vivox runtimes
            if self.prefix(src="../packages/lib/release/vivox-runtime", dst="bin"):
                    self.path("SLVoice")
                    self.end_prefix()
            if self.prefix(src="../packages/lib/release/vivox-runtime", dst="lib32"):
                    self.path("libortp.so")
                    self.path("libvivoxsdk.so")
                    self.end_prefix("lib32")

            # 32bit libs needed for voice
            if self.prefix("../packages/lib/release/32bit-compat", dst="lib32"):
                    self.path("libalut.so")
                    self.path("libalut.so.0")
                    self.path("libidn.so")
                    self.path("libidn.so.11")
                    self.path("libopenal.so")
                    self.path("libopenal.so.1")
                    self.path("libuuid.so")
                    self.path("libuuid.so.1")
                    self.path("libalut.so.0.1.0")
                    self.path("libopenal.so.1.12.854")
                    self.end_prefix("lib32")

        if self.args['buildtype'].lower() == 'release':
                self.run_command('find %(d)r/bin %(d)r/lib32 %(d)r/lib64  -type f \\'
                                 '! -name update_install | xargs --no-run-if-empty strip -S'
                                 % {'d': self.get_dst_prefix()} )

################################################################

if __name__ == "__main__":
    main()
