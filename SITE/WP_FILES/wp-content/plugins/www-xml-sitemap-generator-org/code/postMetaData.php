<?php
namespace xmlSitemapGenerator;

include_once 'dataAccess.php';

class postMetaData
{
 
	function __construct() {
		
 

	}
	
	public static function addHooks() {
		
	//	add_action('load-post.php', array( __CLASS__, 'add_post_hooks' ) );
	//	add_action('load-post-new.php', array( __CLASS__, 'add_post_hooks' ) );	
		add_action('save_post',  array(__CLASS__, 'handlePostBack' ) , 10, 2 );			
		add_action( 'add_meta_boxes', array(__CLASS__, 'addMetaBoxMenu' ) );
		
	}
	
	static function addMetaBoxMenu() {
		
		
		add_meta_box(
			'wpXSG-meta',					// Unique ID
			'XML Sitemap',				// Title
			array(__CLASS__, 'render' ),	// Callback function
			null,						// Screen
			'side',						// Context - note side requires 2.7
			'core'						// Priority
		);
	}

	static function  handlePostBack( $post_id, $post ) {

	
		/* Verify the nonce before proceeding. */
	 	if ( !isset( $_POST['wpXSG_meta_nonce'] ) || !wp_verify_nonce( $_POST['wpXSG_meta_nonce'], basename( __FILE__ ) ) )
	  	return ;
	
	
		/* Get the post type object. */
		$post_type = get_post_type_object( $post->post_type );
		/* Check if the current user has permission to edit the post. */
		if ( !current_user_can( $post_type->cap->edit_post, $post_id ) )
		return $post_id;

		if ( $parent_id = wp_is_post_revision( $post_id ) ) 
		$post_id = $parent_id;

		$settings = new metaSettings();
		$settings->itemId = $post_id;
		$settings->itemType = "post";
		$settings->exclude = ( isset( $_POST['wpXSG-Exclude'] ) ?  $_POST['wpXSG-Exclude'] : '0' );
		$settings->priority = ( isset( $_POST['wpXSG-Priority'] ) ?  $_POST['wpXSG-Priority'] : 'default' );
		$settings->frequency = ( isset( $_POST['wpXSG-Frequency'] ) ?  $_POST['wpXSG-Frequency'] : 'default' );
	

		 dataAccess::saveMetaItem($settings);

	}
	
	
	static function render( $post ) 
	{
	
		
	 	$settings =   dataAccess::getMetaItem($post->ID , "post");  
 
	  wp_nonce_field( basename( __FILE__ ), 'wpXSG_meta_nonce' );
	
		?>
 
		<p><strong>Sitemap inclusion</strong></p>
		<select  name="wpXSG-Exclude" id="wpXSG-Exclude" ></select>
		<label for="wpXSG-Exclude"></label>
		
		<p><strong>Relative priority</strong></p>
		<select  name="wpXSG-Priority" id="wpXSG-Priority" ></select>
 
		<p><strong>Update frequency</strong></p>
		<select  name="wpXSG-Frequency" id="wpXSG-Frequency" ></select>
	


<script type="text/javascript" src="<?php echo xsgPluginPath(); ?>scripts.js"></script>
<script>
  xsg_populate("wpXSG-Exclude" ,excludeSelect, <?php echo $settings->exclude  ?>);
  xsg_populate("wpXSG-Priority" ,prioritySelect, <?php echo $settings->priority  ?>);
  xsg_populate("wpXSG-Frequency" ,frequencySelect, <?php echo $settings->frequency  ?>);
	 
</script>
		<?php

	
	}
 

 

 

}

?>