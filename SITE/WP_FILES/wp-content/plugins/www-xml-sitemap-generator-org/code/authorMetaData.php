<?php
namespace xmlSitemapGenerator;

include_once 'dataAccess.php';

class authorMetaData
{
 
	public static function addHooks()
	{
	 
		add_action('edit_user_profile',  array( __CLASS__, 'renderEdit' ) );
		add_action( 'profile_update'   , array( __CLASS__, 'save_metaData' ), 10, 2); 
		
	}

 
	static function save_metaData($userId ) {
		
		/* Verify the nonce before proceeding. */
	 	if ( !isset( $_POST['wpXSG_meta_nonce'] ) || !wp_verify_nonce( $_POST['wpXSG_meta_nonce'], basename( __FILE__ ) ) )
	  	return ;

	
		/* Check if the current user has permission to edit the post. */
		if ( !current_user_can( 'edit_user') )
		return $userId;

 
	 	$settings = new metaSettings();
		
		$settings->id =  ( isset( $_POST['wpXSG-metaId'] ) ?  $_POST['wpXSG-metaId'] : '0' );
	 	$settings->itemId = $userId ;
		$settings->itemType = "author";
	 	$settings->exclude = ( isset( $_POST['wpXSG-Exclude'] ) ?  $_POST['wpXSG-Exclude'] : '0' );
	 	$settings->priority = ( isset( $_POST['wpXSG-Priority'] ) ?  $_POST['wpXSG-Priority'] : 'default' );
	  	$settings->frequency = ( isset( $_POST['wpXSG-Frequency'] ) ?  $_POST['wpXSG-Frequency'] : 'default' );
		$settings->inherit = ( isset( $_POST['wpXSG-Inherit'] ) ?  $_POST['wpXSG-Inherit'] : 0 );
			
		dataAccess::saveMetaItem($settings );

		
	
	}
	
	
 

	static function renderEdit($WP_User ) 
	{
		$userId = $WP_User->ID;
		self::addHooks();
		
		$settings =  dataAccess::getMetaItem($userId , "author");  
 
	 
			wp_nonce_field( basename( __FILE__ ), 'wpXSG_meta_nonce' );
		?>

		
		<h3>Sitemap settings : 	 </h3>
	


		
		<table class="form-table">
		<tbody><tr class="form-field form-required term-name-wrap">
			<th scope="row"><label for="name">Sitemap inclusion</label></th>
			<td>
				<select  name="wpXSG-Exclude" id="wpXSG-Exclude" ></select>
				<p> Exclude this category/tag from your sitemap.</p>
			</td>
		</tr>
		<tr class="form-field term-slug-wrap">
			<th scope="row"><label for="slug">Relative priority</label></th>
			<td>
				<select  name="wpXSG-Priority" id="wpXSG-Priority" ></select>
				<p>Relative priority for this category/tag and related posts.</p>
			</td>
		</tr>
		<tr class="form-field term-description-wrap">
			<th scope="row"><label for="description">Update frequency</label></th>
			<td>
			<select  name="wpXSG-Frequency" id="wpXSG-Frequency" ></select>
			<p>Sitemap update frequency for this category/tag.</p>
			</td>
		</tr>

		</tbody></table>

<script type="text/javascript" src="<?php echo xsgPluginPath(); ?>scripts.js"></script>
<script>
  xsg_populate("wpXSG-Exclude" ,excludeSelect, <?php echo $settings->exclude  ?>);
  xsg_populate("wpXSG-Priority" ,prioritySelect, <?php echo $settings->priority  ?>);
  xsg_populate("wpXSG-Frequency" ,frequencySelect, <?php echo $settings->frequency  ?>);
  xsg_populate("wpXSG-Inherit" ,inheritSelect, <?php echo $settings->inherit  ?>);
</script>


		<?php
	
	}
 


		
 

}

?>