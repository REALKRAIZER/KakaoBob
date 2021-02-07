<?php
/**
 * The base configuration for WordPress
 *
 * The wp-config.php creation script uses this file during the
 * installation. You don't have to use the web site, you can
 * copy this file to "wp-config.php" and fill in the values.
 *
 * This file contains the following configurations:
 *
 * * MySQL settings
 * * Secret keys
 * * Database table prefix
 * * ABSPATH
 *
 * @link https://wordpress.org/support/article/editing-wp-config-php/
 *
 * @package WordPress
 */

// ** MySQL settings - You can get this info from your web host ** //
/** The name of the database for WordPress */
define( 'DB_NAME', 'u1226956_bobsite' );

/** MySQL database username */
define( 'DB_USER', 'u1226956_admin' );

/** MySQL database password */
define( 'DB_PASSWORD', 'B7e0dSx74d4' );

/** MySQL hostname */
define( 'DB_HOST', 'localhost' );

/** Database Charset to use in creating database tables. */
define( 'DB_CHARSET', 'utf8mb4' );

/** The Database Collate type. Don't change this if in doubt. */
define( 'DB_COLLATE', '' );

/**#@+
 * Authentication Unique Keys and Salts.
 *
 * Change these to different unique phrases!
 * You can generate these using the {@link https://api.wordpress.org/secret-key/1.1/salt/ WordPress.org secret-key service}
 * You can change these at any point in time to invalidate all existing cookies. This will force all users to have to log in again.
 *
 * @since 2.6.0
 */
define( 'AUTH_KEY',         '<`K`X{q)Xgpuo(i1r7J5|q4Xq!ez_a=P-O0y~#FjR5V?p1aFNckd3#xmOAmusovx' );
define( 'SECURE_AUTH_KEY',  'GswTmJ ,wI*z_Bc~LK LGKJKwY5b^CS8G6@gsl2lHaYw`_LYi]MmQ%nS+3vW+C9v' );
define( 'LOGGED_IN_KEY',    'DivKh<8nh;YL1`[hfBKC|N3L[8pV)cDi1@tYF4s)^D=4h>Ocd=`$KWcG.{tXQC8$' );
define( 'NONCE_KEY',        'qprB=;~4f9.PW?@L/!*@$|+Y~yl7]Cyl~[_kKrLy=:qZ%%x/#_$%T+rp6 Yj&?M~' );
define( 'AUTH_SALT',        '[7wle;[YkrEV30U^}%{-#GX`&5^m;+yJkcBynR6uaAr2#*L}!,3i_3BKOMBxWwiU' );
define( 'SECURE_AUTH_SALT', 'hg)tUg`QCyjtL5tg+#t*dntoL-j^.%KCl.cDKef1x%&_sMAG:I_U7-0s.lf~1{*8' );
define( 'LOGGED_IN_SALT',   'l$[Py8;-(RDf?Lye&83r,,=v:-m+$(O9EB!o:CgN,O,8k7b78xhvyB%LsS75p=C-' );
define( 'NONCE_SALT',       'n!8[=BuaCnq@nhf3n>D:4ef>h5=iR<6$960-95;YX[t)amw7zazWlg#jWCM9f2Va' );

/**#@-*/

/**
 * WordPress Database Table prefix.
 *
 * You can have multiple installations in one database if you give each
 * a unique prefix. Only numbers, letters, and underscores please!
 */
$table_prefix = 'wp_';

/**
 * For developers: WordPress debugging mode.
 *
 * Change this to true to enable the display of notices during development.
 * It is strongly recommended that plugin and theme developers use WP_DEBUG
 * in their development environments.
 *
 * For information on other constants that can be used for debugging,
 * visit the documentation.
 *
 * @link https://wordpress.org/support/article/debugging-in-wordpress/
 */
define( 'WP_DEBUG', false );

/* That's all, stop editing! Happy publishing. */

/** Absolute path to the WordPress directory. */
if ( ! defined( 'ABSPATH' ) ) {
	define( 'ABSPATH', __DIR__ . '/' );
}

/** Sets up WordPress vars and included files. */
require_once ABSPATH . 'wp-settings.php';
