<?php
function wamt_network_add_to_settings_menu(){
	if ( ! function_exists( 'is_plugin_active_for_network' ) ) 
		require_once( ABSPATH . '/wp-admin/includes/plugin.php' );	

	if( ! is_multisite() || ! is_plugin_active_for_network(WAMT_PLUGIN_BASENAME))
		return ;
	
	$admin_permission = 'manage_network_options';

    // add_options_page (Title, Setting Title, Permission, Special Definition, function name); 
	add_submenu_page( 'settings.php', __('WP Add Mime Types Admin Settings for Network Administrator', 'wp-add-mime-types'), __('Mime Type Settings','wp-add-mime-types'), $admin_permission, __FILE__, 'wamt_network_admin_settings_page');
}

// Processing Setting menu for the plugin.
function wamt_network_admin_settings_page(){
	$admin_permission = 'manage_network_options';
	// Loading the stored setting data (wp_add_mime_types_network_array) from WordPress database.
	$settings = get_site_option(WAMT_SITEADMIN_SETTING_FILE);

	$permission = false;
	// The user who can manage the WordPress option can only access the Setting menu of this plugin.
	if(current_user_can($admin_permission)) $permission = true; 
	// If the adding data is not set, the value "mime_type_values" sets "empty".
	$mime_type_values = "";
	if(isset($settings['mime_type_values']) && !empty($settings['mime_type_values']))
		$mime_type_values = unserialize($settings['mime_type_values']);
	// When the adding data is saved (posted) at the setting menu, the data will update to the WordPress database after the security check
	if(isset($_POST["wamt-network-form"]) && $_POST["wamt-network-form"]){
		if(check_admin_referer("wamt-network-nonce-key", "wamt-network-form")){
			if(isset($_POST['mime_type_values'])){
				$p_set = esc_attr(strip_tags(html_entity_decode($_POST['mime_type_values']),ENT_QUOTES));
				$mime_type_values = explode("\n", $p_set);
				if(!empty($mime_type_values)){
					foreach($mime_type_values as $m_type=>$m_value)
						// "　" is the Japanese multi-byte space. If the character is found out, it automatically change the space. 
						$mime_type_values[$m_type] = trim(str_replace("　", " ", $m_value));
					$settings['mime_type_values'] = serialize($mime_type_values);
				}
			}
			//else
			//	$mime_type_values = unserialize($settings['mime_type_values']);

			if(!isset($settings['security_attempt_enable']))
				$settings['security_attempt_enable'] = "no";
			else{
				if(isset($_POST['security_attempt_enable']))
					$settings['security_attempt_enable'] = wp_strip_all_tags($_POST['security_attempt_enable']);
			}
			if(!isset($settings['filename_sanitized_enable']))
				$settings['filename_sanitized_enable'] = "no";
			else{
				if(isset($_POST['filename_sanitized_enable']))
					$settings['filename_sanitized_enable'] = wp_strip_all_tags($_POST['filename_sanitized_enable']);
			}
			if(!isset($settings['file_type_debug']))
				$settings['file_type_debug'] = "no";
			else{
				if(isset($_POST['file_type_debug']))
					$settings['file_type_debug'] = wp_strip_all_tags($_POST['file_type_debug']);
			}
			// Update on wp_sitemeta
			update_site_option(WAMT_SITEADMIN_SETTING_FILE, $settings);
		}
	}

?>
<div class="network_add_mime_media_admin_setting_page_updated"><p><strong><?php _e('Updated', 'wp-add-mime-types'); ?></strong></p></div>

<div id="network_add_mime_media_admin_menu">
  <h2><?php _e('WP Add Mime Types Admin Settings for Network Administrator', 'wp-add-mime-types'); ?></h2>

  <form method="post" action="">
	<?php // for CSRF (Cross-Site Request Forgery): https://propansystem.net/blog/2018/02/20/post-6279/
		wp_nonce_field("wamt-network-nonce-key", "wamt-network-form"); ?>
     <fieldset style="border:1px solid #777777; width: 750px; padding-left: 6px;">
		<legend><h3><?php _e('List of allowed mime types and file extensions by WordPress','wp-add-mime-types'); ?></h3></legend>
		<div style="overflow:scroll; height: 500px;">
		<table>
<?php
// Get the list of the file extensions which WordPress allows the upload.
$allowed_mime_values = get_allowed_mime_types();

// Getting the extension name in the saved data
if(!empty($mime_type_values)){
	foreach ($mime_type_values as $line){
		$line_value = explode("=", $line);
		if(count($line_value) != 2) continue;
			$mimes[trim($line_value[0])] = trim($line_value[1]); 
	}
}

// List view of the allowed mime types including the addition (red color) in the admin settings.
if(!empty($allowed_mime_values)){
	foreach($allowed_mime_values as $type=>$value){
		// Escape strings
		$type = wp_strip_all_tags($type);
		$value = wp_strip_all_tags($value);
		if(isset($mimes)){
			$add_mime_type_check = "";
			foreach($mimes as $a_type=>$a_value){
				if(!strcmp($type, $a_type)){  
					$add_mime_type_check = " style='color:red;'";
					break;
				}
			}
			echo "<tr><td$add_mime_type_check>$type</td><td$add_mime_type_check>=</td><td$add_mime_type_check>$value</td></tr>\n";
		}else{
			echo "<tr><td>$type</td><td>=</td><td>$value</td></tr>\n";
		}
	}
}
?>
		</table>
	    </div>
     </fieldset>
	 <br/>

     <fieldset style="border:1px solid #777777; width: 750px; padding-left: 6px; padding-bottom: 1em;">
		<legend><h3><?php _e('Security Options','wp-add-mime-types'); ?></h3></legend>
		<?php  _e('* The plugin avoids some security checks by WordPress core. If you do not want to avoid them, please turn on the following setting.','wp-add-mime-types'); ?></p>

	<?php //  ?>
		<p>
			<input type="hidden" name="security_attempt_enable" value="no" />
			<input type="checkbox" name="security_attempt_enable" value="yes" <?php if( isset($settings['security_attempt_enable']) && $settings['security_attempt_enable'] === "yes" ) echo "checked"; ?> <?php if(!$permission) echo "disabled"; ?>/> <?php _e('Enable the attempt to determine the real file type of a file by WordPress core.','wp-add-mime-types'); ?>
		</p>
		<p>
			<input type="hidden" name="filename_sanitized_enable" value="no" />
			<input type="checkbox" name="filename_sanitized_enable" value="yes" <?php if( isset($settings['filename_sanitized_enable']) && $settings['filename_sanitized_enable'] === "yes" ) echo "checked"; ?> <?php if(!$permission) echo "disabled"; ?>/> <?php _e('Enable to sanitize the multiple file extensions within the filename by WordPress core.','wp-add-mime-types'); ?>
			</p>
		<p>
			<input type="hidden" name="file_type_debug" value="no" />
			<input type="checkbox" name="file_type_debug" value="yes" <?php if( isset($settings['file_type_debug']) && $settings['file_type_debug'] === "yes" ) echo "checked"; ?> <?php if(!$permission) echo "disabled"; ?>/> <?php _e('Enable to debug output for file types recognized by WordPress when a file is uploaded by the media. <br/>* By enabling both this option and the "Enable the attempt to determine the real file type of a file by WordPress core.", the file type is displayed if it is uploaded from Media.<br/>* PLEASE keep in mind that a file uploads are stopped while they are being processed if the both of two options are enabled. Therefore, be sure to disable this debugging option after debugging.','wp-add-mime-types'); ?>
			</p>
     </fieldset>

     <fieldset style="border:1px solid #777777; width: 750px; padding-left: 6px;">
		<legend><h3><?php _e('Add Values','wp-add-mime-types'); ?></h3></legend>
		<p><?php  _e('* About the mime type value for the file extension, please search "mime type [file extension name] using a search engine.<br/>Ex. epub = application/epub+zip<br/>Reference: <a href="http://www.iana.org/assignments/media-types/media-types.xhtml" target="_blank">Media Types on the Internet Assigned Numbers Authority (IANA)</a><br/>* If the added mime type does not work, please deactivate other mime type plugins or the setting of other mime type plugins.','wp-add-mime-types'); ?>
		<br/><?php  _e('* Ignore to the right of "#" on a line. ','wp-add-mime-types'); ?></p>

	<?php // If the permission is not allowed, the user can only read the setting. ?>
		<textarea name="mime_type_values" cols="100" rows="10" <?php if(!$permission) echo "disabled"; ?>><?php if(isset($mimes) && is_array($mimes)) foreach ($mimes as $m_type=>$m_value) echo $m_type . "\t= " .$m_value . "\n"; ?></textarea>
     </fieldset>
     
     <br/>
     
     <input type="submit" value="<?php _e('Save', 'wp-add-mime-types');  ?>" />
  </form>

</div>

<?php
}
