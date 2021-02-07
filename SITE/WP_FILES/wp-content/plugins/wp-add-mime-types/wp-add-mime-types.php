<?php
/*
Plugin Name: WP Add Mime Types 
Plugin URI: 
Description: The plugin additionally allows the mime types and file extensions to WordPress.
Version: 2.5.7
Author: Kimiya Kitani
Author URI: http://kitaney-wordpress.blogspot.jp/
Text Domain: wp-add-mime-types
Domain Path: /lang
*/
define('WAMT_DEFAULT_VAR', '2.5.7');
define('WAMT_PLUGIN_DIR', 'wp-add-mime-types');
define('WAMT_PLUGIN_NAME', 'wp-add-mime-types');
define('WAMT_PLUGIN_BASENAME', WAMT_PLUGIN_DIR . '/' . WAMT_PLUGIN_NAME . '.php');
define('WAMT_SITEADMIN_SETTING_FILE', 'wp_add_mime_types_network_array');
define('WAMT_SETTING_FILE', 'wp_add_mime_types_array');

require_once( dirname( __FILE__  ) . '/includes/admin.php');

// Uninstall settings when the plugin is uninstalled.
function wamt_uninstaller(){
	if(is_multisite())
		delete_site_option(WAMT_SITEADMIN_SETTING_FILE);
	delete_option(WAMT_SETTING_FILE);
}
if ( function_exists('register_uninstall_hook') )
	register_uninstall_hook( __FILE__, 'wamt_uninstaller' );

// Multi-language support.
function wamt_enable_language_translation(){
 load_plugin_textdomain('wp-add-mime-types')
	or load_plugin_textdomain('wp-add-mime-types', false, dirname( WAMT_PLUGIN_BASENAME ) . '/lang/');
}
add_action('plugins_loaded', 'wamt_enable_language_translation');

// Add Setting to WordPress 'Settings' menu for Multisite.
if(is_multisite()){
	add_action('network_admin_menu', 'wamt_network_add_to_settings_menu');
	require_once( dirname( __FILE__  ) . '/includes/network-admin.php');
}
add_action('admin_menu', 'wamt_add_to_settings_menu');

// Procedure for adding the mime types and file extensions to WordPress.
function wamt_add_allow_upload_extension( $mimes ) {
	$mime_type_values = false;

	if ( ! function_exists( 'is_plugin_active_for_network' ) ) 
		require_once( ABSPATH . '/wp-admin/includes/plugin.php' );

	if(is_multisite() && is_plugin_active_for_network(WAMT_PLUGIN_BASENAME))
		$settings = get_site_option(WAMT_SITEADMIN_SETTING_FILE);
	else
		$settings = get_option(WAMT_SETTING_FILE);
		
	if(!isset($settings['mime_type_values']) || empty($settings['mime_type_values'])) return $mimes;
	else
		$mime_type_values = unserialize($settings['mime_type_values']);

	if(!empty($mime_type_values)){
		foreach ((array)$mime_type_values as $line){
			// Ignore to the right of '#' on a line.
			$line = substr($line, 0, strcspn($line, '#'));
			// Escape Strings
			$line = wp_strip_all_tags($line);
			// If 2 or more "=" character in the line data, it will be ignored.
			$line_value = explode("=", $line);
			if(count($line_value) != 2) continue;
			// "　" is the Japanese multi-byte space. If the character is found out, it automatically change the space. 
			$mimes[trim($line_value[0])] = trim(str_replace("　", " ", $line_value[1])); 
		}
	}

	return $mimes;
}

// Register the Procedure process to WordPress.
add_filter( 'upload_mimes', 'wamt_add_allow_upload_extension');

// Using in add_allow_upload_extension_exception function.
function wamt_remove_underscore($filename, $filename_raw){
	return str_replace("_.", ".", $filename);
}
// Exception for WordPress 4.7.1 file contents check system using finfo_file (wp-includes/functions.php)
// In case of custom extension in this plugins' setting, the WordPress 4.7.1 file contents check system is always true.

function wamt_add_allow_upload_extension_exception( $data, $file, $filename,$mimes,$real_mime=null) {
	$mime_type_values = false;

	if(is_multisite() && is_plugin_active_for_network(WAMT_PLUGIN_BASENAME))
		$settings = get_site_option(WAMT_SITEADMIN_SETTING_FILE);
	else
		$settings = get_option(WAMT_SETTING_FILE);

	if ( ! function_exists( 'is_plugin_active_for_network' ) )
		require_once( ABSPATH . '/wp-admin/includes/plugin.php' );

	if(!isset($settings['mime_type_values']) || empty($settings['mime_type_values'])) 
		return compact( 'ext', 'type', 'proper_filename' );
	else
		$mime_type_values = unserialize($settings['mime_type_values']);

	$ext = $type = $proper_filename = false;
	if(isset($data['ext'])) $ext = $data['ext'];
	if(isset($data['type'])) $ext = $data['type'];
	if(isset($data['proper_filename'])) $ext = $data['proper_filename'];
	if($ext != false && $type != false) return $data;	

	// If file extension is 2 or more 
	$f_sp = explode(".", $filename);
	$f_exp_count  = count ($f_sp);

	// Filename type is "XXX" (There is not file extension). 
	if($f_exp_count <= 1){
		return $data;
	/* Even if the file extension is "XXX.ZZZ", "XXX.YYY.ZZZ", "AAA.XXX.YYY.ZZZ" or more, it always picks up  the tail of the extensions.
	*/
	}else{
		$f_name = $f_sp[0];
		$f_ext  = $f_sp[$f_exp_count - 1];
		// WordPress sanitizes the filename in case of 2 or more extensions. 
		// ex. XXX.YYY.ZZZ --> XXX_.YYY.ZZZ.
		// The following function fixes the sanitized extension when a file is uploaded in the media in case of allowed extensions. 
		// ex. XXX.YYY.ZZZ -- sanitized --> XXX_.YYY.ZZZ -- fixed the plugin --> XXX.YYY.ZZZ
		// In detail, please see sanitize_file_name function in "wp-includes/formatting.php".
		//var_dump($settings['filename_sanitized_enable']);
		if(isset($settings['filename_sanitized_enable']) && $settings['filename_sanitized_enable'] === "yes"){
		}else{
			add_filter( 'sanitize_file_name', 'wamt_remove_underscore', 10, 2 );
		}
	}

	// If "security_attempt_enable" option disables (default) in the admin menu, the plugin avoids the security check regarding a file extension by WordPress core because of flexible management.
	if(isset($settings['security_attempt_enable']) && $settings['security_attempt_enable'] === "yes"){
		if(isset($settings['file_type_debug']) && $settings['file_type_debug'] === "yes"):
			$finfo     = finfo_open( FILEINFO_MIME_TYPE );
			$real_mime = finfo_file($finfo,$file);
			var_dump(__("WordPress recognizes that the file type is [". finfo_file($finfo,$file) . "].",'wp-add-mime-types'));
			finfo_close($finfo);
		endif;
		return $data;
	}

	$flag = false;
	if(!empty($mime_type_values)){
		foreach ((array)$mime_type_values as $line){
			// Ignore to the right of '#' on a line.
			$line = substr($line, 0, strcspn($line, '#'));
			// Escape Strings
			$line = wp_strip_all_tags($line);
			
			$line_value = explode("=", $line);
			if(count($line_value) != 2) continue;
			// "　" is the Japanese multi-byte space. If the character is found out, it automatically change the space. 		
			if(trim($line_value[0]) === $f_ext){
				$ext = $f_ext;
				$type = trim(str_replace("　", " ", $line_value[1])); 
				$flag = true;
				break;
			}
		}
	}

	if($flag)
	    return compact( 'ext', 'type', 'proper_filename' );
	else
		return $data;
}

// It's different arguments between WordPress 5.1 and previous versions.
global $wp_version;
if ( version_compare( $wp_version, '5.1') >= 0):
	add_filter( 'wp_check_filetype_and_ext', 'wamt_add_allow_upload_extension_exception',10,5);
else:
	add_filter( 'wp_check_filetype_and_ext', 'wamt_add_allow_upload_extension_exception',10,4);
endif;