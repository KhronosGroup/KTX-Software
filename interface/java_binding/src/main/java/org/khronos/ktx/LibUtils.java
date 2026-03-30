/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file was originally part of JOCL, from https://github.com/gpu/JOCL,
 * as of commit 923eec9ce77b26d9355c864cb1db712889ddb8ec, and is published
 * by the author (with minor modifications) as part of KTX-Software, under 
 * the Apache-2.0 license.
 * 
 * Original license header:
 * 
 * JOCL - Java bindings for OpenCL
 *
 * Copyright (c) 2009-2015 Marco Hutter - http://www.jocl.org
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

package org.khronos.ktx;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Locale;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Utility class for detecting the operating system and architecture
 * types, and automatically loading the matching native library
 * as a resource or from a file. <br>
 * <br>
 * This class is not intended to be used by clients.<br>
 * <br>
 */
public final class LibUtils
{
    // The architecture and OS detection has been adapted from 
    // http://javablog.co.uk/2007/05/19/making-jni-cross-platform/
    // and extended with http://lopica.sourceforge.net/os.html 
    
    /**
     * The logger used in this class
     */
    private static final Logger logger =
        Logger.getLogger(LibUtils.class.getName());
    
    /**
     * The default log level
     */
    private static final Level level = Level.INFO;

    /**
     * The directory where libraries are expected in JAR files,
     * when they are loaded as resources
     */
    private static final String LIBRARY_PATH_IN_JAR = "/lib";
    
    /**
     * Enumeration of common operating systems, independent of version 
     * or architecture. 
     */
    enum OSType
    {
        /**
         * Android
         */
        ANDROID, 
        
        /**
         * Apple/MacOS
         */
        APPLE, 
        
        /**
         * Linux 
         */
        LINUX, 
        
        /**
         * Sun. Solaris, probably...
         */
        SUN, 
        
        /**
         * Windows
         */
        WINDOWS, 
        
        /**
         * Unknown 
         */
        UNKNOWN
    }

    /**
     * Enumeration of common CPU architectures.
     */
    enum ArchType
    {
        /**
         * Power PC (32 bit)
         */
        PPC, 
        
        /**
         * Power PC, 64 bit
         */
        PPC_64, 
        
        /**
         * SPARC
         */
        SPARC, 
        
        /**
         * x86 (32 bit)
         */
        X86, 
        
        /**
         * x86 (64 bit)
         */
        X86_64, 
        
        /**
         * ARM (32 bit)
         */
        ARM, 
        
        /**
         * ARM (64 bit)
         */
        ARM64, 
        
        /**
         * MIPS (32 bit)
         */
        MIPS, 
        
        /**
         * MIPS (64 bit)
         */
        MIPS64, 
        
        /**
         * RISC
         */
        RISC, 
        
        /**
         * Unknown
         */
        UNKNOWN
    }

    /**
     * Private constructor to prevent instantiation.
     */
    private LibUtils()
    {
        // Private constructor to prevent instantiation.
    }

    /**
     * Loads the specified library. <br>
     * <br>
     * The method will attempt to load the library using the usual 
     * <code>System.loadLibrary</code> call. In this case, the specified 
     * dependent libraries are ignored, because they are assumed to be 
     * loaded automatically in the same way as the main library.<br>
     * <br> 
     * If the library can <b>not</b> be loaded with the 
     * <code>System.loadLibrary</code> call, then this method will attempt
     * to load the file as a resource (usually one that is contained in
     * a JAR file). In this case, the library is assumed to be located
     * in subdirectory called <code>"/lib"</code> inside the JAR file. 
     * The method will try to load a resource that has the platform-specific 
     * {@link #createLibraryFileName(String) library file name} from 
     * this directory, extract it into the default directory for temporary
     * files, and load the library from there. <br>
     * <br>
     * In this case, the specified dependent libraries may also be loaded 
     * as resources. They are assumed to be located in subdirectories
     * that are named according to the {@link #osString()} and 
     * {@link #archString()} of the executing platform. For example, such
     * a library may be located in a directory inside the JAR that is
     * called <code>"/lib/windows/x86_64"</code>. These dependent libraries 
     * will be extracted and loaded before the main library is loaded. 
     *    
     * @param libraryName The name of the library (without a platform specific
     * prefix or file extension)
     * @param dependentLibraryNames The names of libraries that the library
     * to load depends on. If the library is loaded as a resource, then 
     * it will be attempted to also load these libraries as resources, as
     * described above 
     * @throws UnsatisfiedLinkError if the native library 
     * could not be loaded.
     */
    public static void loadLibrary(
        String libraryName, String ... dependentLibraryNames)
    {
        logger.log(level, "Loading library: " + libraryName);

        // First, try to load the specified library as a file 
        // that is visible in the default search path
        Throwable throwableFromFile;
        try
        {
            logger.log(level, "Loading library as a file");
            System.loadLibrary(libraryName);
            logger.log(level, "Loading library as a file DONE");
            return;
        }
        catch (Throwable t)
        {
            logger.log(level, "Loading library as a file FAILED");
            throwableFromFile = t;
        }

        // Now try to load the library by extracting the
        // corresponding resource from the JAR file
        try
        {
            logger.log(level, "Loading library as a resource");
            loadLibraryResource(LIBRARY_PATH_IN_JAR, 
                libraryName, "", dependentLibraryNames);
            logger.log(level, "Loading library as a resource DONE");
            return;
        }
        catch (Throwable throwableFromResource)
        {
            logger.log(level, "Loading library as a resource FAILED", 
                throwableFromResource);

            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);

            pw.println("Error while loading native library \"" +
                libraryName + "\"");
            pw.println("Operating system name: "+
                System.getProperty("os.name"));
            pw.println("Architecture         : "+
                System.getProperty("os.arch"));
            pw.println("Architecture bit size: "+
                System.getProperty("sun.arch.data.model"));

            pw.println("---(start of nested stack traces)---");
            
            pw.println("Stack trace from the attempt to " +
                "load the library as a file:");
            throwableFromFile.printStackTrace(pw);

            pw.println("Stack trace from the attempt to " +
                "load the library as a resource:");
            throwableFromResource.printStackTrace(pw);
            
            pw.println("---(end of nested stack traces)---");

            pw.close();
            throw new UnsatisfiedLinkError(sw.toString());
        }
    }




    /**
     * Load the library with the given name from a resource. 
     * 
     * @param resourceSubdirectoryName The subdirectory where the resource
     * is expected
     * @param libraryName The library name, e.g. "EXAMPLE-windows-x86"
     * @param tempSubdirectoryName The name for the subdirectory in the
     * temp directory, where the temporary files for dependent libraries
     * should be stored
     * @param dependentLibraryNames The names of libraries that the library
     * to load depends on, and that may have to be loaded as resources and
     * stored as temporary files as well 
     * @throws Throwable If the library could not be loaded
     */
    private static void loadLibraryResource(
        String resourceSubdirectoryName,
        String libraryName,
        String tempSubdirectoryName,
        String ... dependentLibraryNames) throws Throwable
    {
        // First try to load all dependent libraries, recursively
        for (String dependentLibraryName : dependentLibraryNames)
        {
            logger.log(level, 
                "Library " + libraryName + 
                " depends on " + dependentLibraryName);
            
            String dependentResourceSubdirectoryName =
                resourceSubdirectoryName + "/" + 
                osString() + "/" + 
                archString();

            String dependentLibraryTempSubDirectoryName =
                libraryName+"_dependents" + File.separator + 
                osString() + File.separator +
                archString() + File.separator;
            
            loadLibraryResource(
                dependentResourceSubdirectoryName,
                dependentLibraryName, 
                dependentLibraryTempSubDirectoryName);
        }
        
        // Now, prepare loading the actual library
        String libraryFileName = createLibraryFileName(libraryName);
        File libraryTempFile = createTempFile(
            tempSubdirectoryName, libraryFileName);
        
        // If the temporary file for the library does not exist, create it
        if (!libraryTempFile.exists())
        {
            String libraryResourceName = 
                resourceSubdirectoryName + "/" + libraryFileName;
            logger.log(level, 
                "Writing resource  " + libraryResourceName);
            logger.log(level, 
                "to temporary file " + libraryTempFile);
            writeResourceToFile(libraryResourceName, libraryTempFile);
        }
        
        // Finally, try to load the library from the temporary file
        logger.log(level, "Loading library " + libraryTempFile);
        System.load(libraryTempFile.toString());
        logger.log(level, "Loading library " + libraryTempFile + " DONE");
    }
    

    /**
     * Create a file object representing the file with the given name
     * in the specified subdirectory of the default "temp" directory. 
     * If the specified subdirectory does not exist yet, it is created.
     *  
     * @param tempSubdirectoryName The subdirectory name 
     * @param name The file name
     * @return The file
     * @throws IOException If the subdirectory can not be created
     */
    private static File createTempFile(
        String tempSubdirectoryName, String name) throws IOException
    {
        String tempDirName = System.getProperty("java.io.tmpdir");
        File tempSubDirectory = 
            new File(tempDirName + File.separator + tempSubdirectoryName);
        if (!tempSubDirectory.exists())
        {
            boolean createdDirectory = tempSubDirectory.mkdirs();
            if (!createdDirectory)
            {
                throw new IOException(
                    "Could not create directory for temporary file: " + 
                     tempSubDirectory);
            }
        }
        String tempFileName = tempSubDirectory + File.separator + name;
        File tempFile = new File(tempFileName);
        return tempFile;
    }
    
    
    /**
     * Obtain an input stream to the resource with the given name, and write 
     * it to the specified file (which may not be <code>null</code>, and 
     * may not exist yet)
     * 
     * @param resourceName The name of the resource
     * @param file The file to write to
     * @throws NullPointerException If the given file is <code>null</code>
     * @throws IllegalArgumentException If the given file already exists
     * @throws IOException If an IO error occurs
     */
    private static void writeResourceToFile(
        String resourceName, File file) throws IOException
    {
        if (file == null)
        {
            throw new NullPointerException("Target file may not be null");
        }
        if (file.exists())
        {
            throw new IllegalArgumentException(
                "Target file already exists: "+file);
        }
        InputStream inputStream = 
            LibUtils.class.getResourceAsStream(resourceName);
        if (inputStream == null)
        {
            throw new IOException(
                "No resource found with name '"+resourceName+"'");
        }
        OutputStream outputStream = null;
        try
        {
            outputStream = new FileOutputStream(file);
            byte[] buffer = new byte[32768];
            while (true)
            {
                int read = inputStream.read(buffer);
                if (read < 0)
                {
                    break;
                }
                outputStream.write(buffer, 0, read);    
            }
            outputStream.flush();
        }
        finally 
        {
            if (outputStream != null)
            {
                try
                {
                    outputStream.close();
                }
                catch (IOException e)
                {
                    logger.log(Level.SEVERE, e.getMessage(), e);
                }
            }
            try
            {
                inputStream.close();
            }
            catch (IOException e)
            {
                logger.log(Level.SEVERE, e.getMessage(), e);
            }
        }
    }
    

    /**
     * Create the full library file name, including the extension
     * and prefix, for the given library name. For example, the
     * name "EXAMPLE" will become <br>
     * EXAMPLE.dll on Windows <br>
     * libEXAMPLE.so on Linux <br>
     * EXAMPLE.dylib on MacOS <br>
     * 
     * @param libraryName The library name
     * @return The full library name, with extension
     */
    public static String createLibraryFileName(String libraryName)
    {
        String libPrefix = createLibraryPrefix();
        String libExtension = createLibraryExtension();
        String fullName = libPrefix + libraryName + "." + libExtension;
        return fullName;
    }


    /**
     * Returns the extension for dynamically linked libraries on the
     * current OS. That is, returns <code>"dylib"</code> on Apple, 
     * <code>"so"</code> on Linux and Sun, and <code>"dll"</code> 
     * on Windows.
     * 
     * @return The library extension
     */
    private static String createLibraryExtension()
    {
        OSType osType = calculateOS();
        switch (osType) 
        {
            case APPLE:
                return "dylib";
            case ANDROID:
            case LINUX:
            case SUN:
                return "so";
            case WINDOWS:
                return "dll";
            default:
                break;
        }
        return "";
    }

    /**
     * Returns the prefix for dynamically linked libraries on the
     * current OS. That is, returns <code>"lib"</code> on Apple, 
     * Linux and Sun, and the empty String on Windows.
     * 
     * @return The library prefix
     */
    private static String createLibraryPrefix()
    {
        OSType osType = calculateOS();
        switch (osType) 
        {
            case ANDROID:
            case APPLE:
            case LINUX:
            case SUN:
                return "lib";
            case WINDOWS:
                return "";
            default:
                break;
        }
        return "";
    }


    /**
     * Creates the name for the native library with the given base name for 
     * the current platform, by appending strings that indicate the current 
     * operating system and architecture.<br>
     * <br>
     * The resulting name will be of the form<br>
     * <code>baseName-OSType-ArchType</code><br>
     * where OSType and ArchType are the <strong>lower case</strong> Strings
     * of the respective {@link LibUtils.OSType OSType} and 
     * {@link LibUtils.ArchType ArcType} enum constants.<br>
     * <br> 
     * For example, the library name with the base name "EXAMPLE" may be<br>
     * <code>EXAMPLE-windows-x86</code><br>
     * <br>
     * Note that the resulting name will not include any platform specific
     * prefixes or extensions for the actual name.  
     * 
     * @param baseName The base name of the library
     * @return The library name
     */
    public static String createPlatformLibraryName(String baseName)
    {
        return baseName + "-" + osString() + "-" + archString();
    }
    
    /**
     * Returns a the <strong>lower case</strong> String representation of
     * the {@link #calculateOS() OSType} of this platform. E.g. 
     * <code>"windows"</code>.
     * 
     * @return The string describing the operating system
     */
    private static String osString()
    {
        OSType osType = calculateOS();
        return osType.toString().toLowerCase(Locale.ENGLISH);
    }
    
    /**
     * Returns a the <strong>lower case</strong> String representation of
     * the {@link #calculateArch() ArchType} of this platform. E.g. 
     * <code>"x86_64"</code>.
     * 
     * @return The string describing the architecture
     */
    private static String archString()
    {
        ArchType archType = calculateArch();
        return archType.toString().toLowerCase(Locale.ENGLISH);
    }

    /**
     * Calculates the current OSType
     * 
     * @return The current OSType
     */
    static OSType calculateOS()
    {
        String vendor = System.getProperty("java.vendor");
        if ("The Android Project".equals(vendor))
        {
            return OSType.ANDROID;
        }
        String osName = System.getProperty("os.name");
        osName = osName.toLowerCase(Locale.ENGLISH);
        if (osName.startsWith("mac os"))
        {
            return OSType.APPLE;
        }
        if (osName.startsWith("windows"))
        {
            return OSType.WINDOWS;
        }
        if (osName.startsWith("linux"))
        {
            return OSType.LINUX;
        }
        if (osName.startsWith("sun"))
        {
            return OSType.SUN;
        }
        return OSType.UNKNOWN;
    }


    /**
     * Calculates the current ARCHType
     * 
     * @return The current ARCHType
     */
    private static ArchType calculateArch()
    {
        String osArch = System.getProperty("os.arch");
        osArch = osArch.toLowerCase(Locale.ENGLISH);
        if ("i386".equals(osArch) ||
            "x86".equals(osArch)  ||
            "i686".equals(osArch))
        {
            return ArchType.X86; 
        }
        if (osArch.startsWith("amd64") || osArch.startsWith("x86_64"))
        {
            return ArchType.X86_64;
        }
        if (osArch.startsWith("arm64") || osArch.equals("aarch64"))
        {
            return ArchType.ARM64;
        }
        if (osArch.startsWith("arm"))
        {
            return ArchType.ARM;
        }
        if ("ppc".equals(osArch) || "powerpc".equals(osArch))
        {
            return ArchType.PPC;
        }
        if (osArch.startsWith("ppc"))
        {
            return ArchType.PPC_64;
        }
        if (osArch.startsWith("sparc"))
        {
            return ArchType.SPARC;
        }
        if (osArch.startsWith("mips64"))
        {
            return ArchType.MIPS64;
        }
        if (osArch.startsWith("mips"))
        {
            return ArchType.MIPS;
        }
        if (osArch.contains("risc"))
        {
            return ArchType.RISC;
        }
        return ArchType.UNKNOWN;
    }
}