<?php
?>
<div class="hours_day">
	<div class="dukeusers">
		<div class="hours"><?php echo $item->hours; ?></div>
		<span class="title"><?php echo $item->libraryDate['dayAndDate']; ?></span>
	</div>
	<?php if(!empty($public_item)): ?>
	<div class="genpublic">Public: <?php echo $public_item->hours; ?></div>
	<?php endif; ?>
</div>
