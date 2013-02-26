<?php

function dul_hours_load_calendar_service() {
	static $calendar_service;
	if (!isset($calendar_service)) {
		date_default_timezone_set('America/New_York');
		$apiClient = new apiClient();
		$apiClient->setUseObjects(TRUE);
		$calendar_service = new apiCalendarService($apiClient);
	}
	return $calendar_service;
}

/**
 * Fetch days from a Google Calendar Service
 *
 * @param $calID 
 *  google calendar ID
 
 * @param $service
 *  handle to Google Calendar API service
 *
 * @param $day_start
 *  first day to fetch events (timestamp or string)
 *
 * @param $num_events
 *  how many events
 */
function dul_hours_fetch_events($calId, $service, $day_start, $num_events) {
	$timeMin = '';
	$singleEvents = variable_get('dul_hours.single_events', TRUE);
	$orderBy = 'startTime';

	if (!isset($day_start)) {
		$day_start = time();	// now
	}

	if (is_int($day_start)) {
		// convert to date string format (perferred)
		$timeMin = date('Y-m-d', $day_start) . 'T00:00:00-05:00';
	}

	$params = array(
		'timeMin' => $timeMin,
		'singleEvents' => $singleEvents,
		'orderBy' => $orderBy,
		'maxResults' => $num_events,
	);

	echo sprintf("%s@group.calendar.google.com\n", $calId);

	$events = $service->events->listEvents($calId . '@group.calendar.google.com', $params);
	return $events;
}
