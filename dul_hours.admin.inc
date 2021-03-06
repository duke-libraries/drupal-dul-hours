<?php

/**
 * @file
 * Administrative tasks, pages and/or forms to manage
 * the Library Hours interface
 */

/**
 * Form builder. Configure annotations.
 *
 * @ingroup forms
 * @see system_settings_form()
 */
function dul_hours_admin_settings() {
	// get the list of libraries
	$libraries = dul_hours_get_libraries();
	$options = array();
	foreach ($libraries as $library) {
		$options[$library->library_id] = $library->library_name;
	}

	$form['dul_hours_default_library'] = array(
		'#type' => 'select',
		'#title' => t('Library for which to display hours on front page'),
		'#options' => $options,
		'#default_value' => variable_get('dul_hours.default_library', ''),
		'#description' => t('When the hours are displayed as a block, this will be the library used.'),
	);

	$form['dul_hours_default_num_events'] = array(
		'#type' => 'textfield',
		'#title' => t('Default Number of Events to fetch for blocks'),
		'#default_value' => variable_get('dul_hours.default_num_events', ''),
		'#required' => TRUE,
		'#size' => 4,
	);

	$form['dul_hours_public_hours_news_url'] = array(
		'#type' => 'textfield',
		'#title' => t('Public Hours/Card-Access Entry Points URL'),
		'#description' => t('Relative URL to the page that explains public hours and card access'),
		'#default_value' => variable_get('dul_hours.publichours_cardaccess_url', ''),
	);
	
	$default_single_events = variable_get('dul_hours.default_single_events', TRUE) ? '1' : '0';
	$form['dul_hours_default_single_events'] = array(
		'#type' => 'checkbox',
		'#title' => t('Fetch single events per day'),
		'#default_value' => $default_single_events,
		'#description' => t('Recommened'),
	);

	// Originally, I thought having a "refresh interval" setting 
	// to update the hours data would be a good, logical idea.  
	// However, a potential site response 'lag' could occur when a user 
	// visits the home page (or any page) during a time when a 
	// database refresh took place.
	//
	// This, of course, would degrade our usability factor.
	// So, a solution would install a 'true' cron job on the VMs
	// that would invoke a 'drush' command to refresh the hours data
	// preferrably after hours.

	$form['#submit'][] = 'dul_hours_admin_settings_submit';
	return system_settings_form($form);
}

/**
 * Process dul_hours settings submission.
 */
function dul_hours_admin_settings_submit($form, $form_state) {
	variable_set('dul_hours.default_library', $form_state['values']['dul_hours_default_library']);
	variable_set('dul_hours.default_num_events', $form_state['values']['dul_hours_default_num_events']);
	variable_set('dul_hours.publichours_cardaccess_url', $form_state['values']['dul_hours_public_hours_news_url']);
}

/**
 * Form builder for Library listing
 *
 * @ingroup forms
 * @see drupal_get_form
 */
function dul_hours_admin_library_default($form, &$form_state) {
	# get the list of libraries
	error_log('in form default');
	$libraries = dul_hours_get_libraries();

	$headers = array('Library Name', 'Library ID', 'Google Calendar ID', 'Actions');
	$rows = array();
	foreach ($libraries as $library) {
		$row = array();	// initialize the new array
		$row[] = $library->library_name;
		$row[] = $library->library_id;
		$row[] = $library->google_cal_id;
		$row[] = l(t('Edit'), 'admin/config/library_hours/library/edit/' . $library->lid);
		$rows[] = $row;
	}
	$form['library_table'] = array(
		'#theme' => 'table',
		'#header' => $headers,
		'#rows' => $rows,
	);

	$form['add_library'] = array(
		'#type' => 'fieldset',
		'#title' => t('Add a Library Mapping'),
		'#description' => t('Use this section to add a Library-to-Google Calendar mapping'),
		'#collapsible' => TRUE,
		'#collapsed' => FALSE,
	);
	$form['add_library']['library_id'] = array(
		'#type' => 'textfield',
		'#title' => t('Library ID'),
		'#description' => t('Examples: <em>perkins</em>, <em>rubenstein</em> -- Please use lowercase letters and no spaces'),
		'#required' => TRUE,
	);
	$form['add_library']['library_name'] = array(
		'#type' => 'textfield',
		'#title' => t('Library Name'),
		'#description' => t('Enter a descriptive name for this record.'),
		'#required' => TRUE,
	);
	$form['add_library']['google_cal_id'] = array(
		'#type' => 'textfield',
		'#title' => t('Google Calendar ID'),
		'#required' => TRUE,
	);	
	$form['add_library']['expose_block'] = array(
		'#type' => 'checkbox',
		'#title' => t('Expose <em>Upcoming Days</em> Block for this calendar'),
	);
	$form['add_library']['actions'] = array(
		'#type' => 'actions',
	);
	$form['add_library']['actions']['add_library'] = array(
		'#type' => 'submit',
		'#value' => t('Add Record'),
		'#submit' => array('dul_hours_add_library_submit'),
	);
	return $form;
}

/**
 * Handle post-validation form submission for adding library
 * @ingroup forms
 */
function dul_hours_add_library_submit($form, $form_state) {
	# validation is assumed to have passed, so it's safe 
	# to save the new record
	# --
	# however, do a check to make sure it doesn't already exist.
	
	$res = db_query('SELECT * FROM {duke_cal_library} WHERE library_id = :library_id',
		array(':library_id' => $form_state['values']['library_id']));
	if ($res->fetchObject()) {
		drupal_set_message(t('This Library instance already exists.'), 'error');
		return;
	}
	try {
		$q = db_insert('duke_cal_library')
			->fields(
				array(
					'library_id' => $form_state['values']['library_id'],
					'library_name' => check_plain($form_state['values']['library_name']),
					'google_cal_id' => check_plain($form_state['values']['google_cal_id']),
				)
			);
		if ($q->execute()) {
			drupal_set_message('The new record was added successfully');			
		}
		$dul_hours_cal_blocks = variable_get('dul_hours.upcoming_days_block', array());
		if (!in_array($form_state['values']['library_id'], $dul_hours_cal_blocks)) {
			$dul_hours_cal_blocks[] = $form_state['values']['library_id'];
		}
		variable_set('dul_hours.upcoming_days_block', $dul_hours_cal_blocks);
	} catch(Exception $e) {
		drupal_set_message($e->getMessage(), 'error');
	}
}

/**
 * Form builder for Library Edit
 *
 * @ingroup forms
 * @see drupal_get_form
 */
function dul_hours_edit_library($form, &$form_state, $library) {
	$dul_hours_cal_blocks = variable_get('dul_hours.upcoming_days_block', array());
	
	$form['library_name'] = array(
		'#type' => 'textfield',
		'#default_value' => decode_entities(check_plain($library->library_name)),
		'#size' => 40,
		'#title' => t('Library Name'),
		'#description' => t('The display name of this library when viewed on a page that displays Hours information'),
		'#required' => TRUE,
	);
	$form['lid'] = array(
		'#type' => 'hidden',
		'#value' => $library->lid,
	);
	$form['library_id'] = array(
		'#type' => 'textfield',
		'#size' => 15,
		'#required' => TRUE,
		'#title' => t('Library ID'),
		'#default_value' => $library->library_id,
	);
	$form['google_cal_id'] = array(
		'#type' => 'textfield',
		'#size' => 60,
		'#title' => t('Google Calendar ID'),
		'#description' => t('Enter the calendar identifier from Google.  Used when fetching hours data.'),
		'#default_value' => $library->google_cal_id,
	);
	$form['expose_block'] = array(
		'#type' => 'checkbox',
		'#title' => t('Expose <em>Upcoming Days</em> Block for this calendar'),
		'#default_value' => in_array($library->library_id, $dul_hours_cal_blocks) ? 1 : 0,
	);
	$form['actions'] = array(
		'#type' => 'actions',
	);
	$form['actions']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Save Changes'),
	);
	return $form;
}

/**
 * Submit function for dul_hours_admin_edit_library
 */
function dul_hours_edit_library_submit($form, $form_state) {
	$o = new stdClass();
	$o->lid = $form_state['values']['lid'];
	$o->library_id = check_plain($form_state['values']['library_id']);
	$o->google_cal_id = check_plain($form_state['values']['google_cal_id']);
	$o->library_name = decode_entities(check_plain($form_state['values']['library_name']));
	if (drupal_write_record('duke_cal_library', $o, 'lid')) {
		drupal_set_message(t('The Library/Google Calendar Mapping has been saved'));
		
		$dul_hours_cal_blocks = variable_get('dul_hours.upcoming_days_block', array());
		if (!in_array($form_state['values']['library_id'], $dul_hours_cal_blocks)) {
			$dul_hours_cal_blocks[] = $form_state['values']['library_id'];
		}
		variable_set('dul_hours.upcoming_days_block', $dul_hours_cal_blocks);
		
	} else {
		drupal_set_message(t('Unable to save Library/Google Calendar Mapping'), 'error');
	}
	

}
