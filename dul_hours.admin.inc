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
}

/**
 * Form builder for Library listing
 *
 * @ingroup forms
 * @see drupal_get_form
 */
function dul_hours_admin_library_default() {
	# get the list of libraries
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
	return array(
		'library_table' => array(
			'#theme' => 'table',
			'#header' => $headers,
			'#rows' => $rows,
		),
	);
}

/**
 * Form builder for Library Edit
 *
 * @ingroup forms
 * @see drupal_get_form
 */
function dul_hours_admin_edit_library($library) {
	$form = array();
	
	$form['library_name'] = array(
		'#type' => 'textfield',
		'#default_value' => check_plain($library->library_name),
		'#size' => 40,
		'#title' => t('Library Name'),
		'#description' => t('The display name of this library when viewed on a page that displays Hours information'),
		'#required' => TRUE,
	);
	$form['lid'] = array(
		'#type' => 'hidden',
		'#value' => $library->lid,
	);
	$form['library_id'] => array(
		'#type' => 'textfield',
		'#size' => 15,
		'#required' => TRUE,
		'#title' => t('Library ID'),
		'#default_value' => $library->library_id,
	),
	$form['google_cal_id'] = array(
		'#type' => 'textfield',
		'#size' => 60,
		'#title' => t('Google Calendar ID'),
		'#description' => t('Enter the calendar identifier from Google.  Used when fetching hours data.'),
		'#default_value' => $library->google_cal_id,
	);
	return $form;
}
