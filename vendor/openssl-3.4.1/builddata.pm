package OpenSSL::safe::installdata;

use strict;
use warnings;
use Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(
    @PREFIX
    @libdir
    @BINDIR @BINDIR_REL_PREFIX
    @LIBDIR @LIBDIR_REL_PREFIX
    @INCLUDEDIR @INCLUDEDIR_REL_PREFIX
    @APPLINKDIR @APPLINKDIR_REL_PREFIX
    @ENGINESDIR @ENGINESDIR_REL_LIBDIR
    @MODULESDIR @MODULESDIR_REL_LIBDIR
    @PKGCONFIGDIR @PKGCONFIGDIR_REL_LIBDIR
    @CMAKECONFIGDIR @CMAKECONFIGDIR_REL_LIBDIR
    $VERSION @LDLIBS
);

our @PREFIX                     = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1' );
our @libdir                     = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1' );
our @BINDIR                     = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\apps' );
our @BINDIR_REL_PREFIX          = ( 'apps' );
our @LIBDIR                     = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1' );
our @LIBDIR_REL_PREFIX          = ( '' );
our @INCLUDEDIR                 = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\include', 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\include' );
our @INCLUDEDIR_REL_PREFIX      = ( 'include', './include' );
our @APPLINKDIR                 = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\ms' );
our @APPLINKDIR_REL_PREFIX      = ( 'ms' );
our @ENGINESDIR                 = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\engines' );
our @ENGINESDIR_REL_LIBDIR      = ( 'engines' );
our @MODULESDIR                 = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1\providers' );
our @MODULESDIR_REL_LIBDIR      = ( 'providers' );
our @PKGCONFIGDIR               = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1' );
our @PKGCONFIGDIR_REL_LIBDIR    = ( '.' );
our @CMAKECONFIGDIR             = ( 'C:\Users\Ty Qualters\Documents\GitHub\cpkg\vendor\openssl-3.4.1' );
our @CMAKECONFIGDIR_REL_LIBDIR  = ( '.' );
our $VERSION                    = '3.4.1';
our @LDLIBS                     =
    # Unix and Windows use space separation, VMS uses comma separation
    $^O eq 'VMS'
    ? split(/ *, */, 'ws2_32.lib gdi32.lib advapi32.lib crypt32.lib user32.lib ')
    : split(/ +/, 'ws2_32.lib gdi32.lib advapi32.lib crypt32.lib user32.lib ');

1;
