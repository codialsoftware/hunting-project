function gallery_summary_rows() {
	return $('.photo-gallery-row');
}

function make_gallery_row($item) {
	return $item
		.wrap('<tr class="photo-gallery-title-row"><td colspan="2"></td></tr>');
}

function move_element($item, callback) {
	callback($item.parent().parent().detach());
	return $item;
}

function cut_text_noword_cut(inText, maxlength) {
	if (inText.length <= maxlength)
		return inText;

	var regex = /\s/g;
	var lastWS = 0;
	for (var m; (m = regex.exec(inText)) && (m.index < maxlength); lastWS = m.index)
		; // intended empty loop
		
	return inText.substr(0, lastWS);
}

function brief_text(item, maxlength) {
	var text = $(item).html();
	
	var cuttext = cut_text_noword_cut(text, maxlength - 4) + ' ...';
	$(item).html(cuttext);
}

function node_gallery_cheats() {
	var $rows = gallery_summary_rows();
	
	$rows.each(
		function() {
			var $row = $(this);
			var $items = $('td:first div', $row);
			var $cimg = $items[1];
			move_element(
				make_gallery_row($($items[0])), 
				function ($item) {
					$item.insertBefore($row);
				})
				.wrap('<h2 />');
				
			move_element(
				make_gallery_row($($items[2])),
				function ($item) {
					$item.insertAfter($row);
				});
				
			brief_text($('td.photo-gallery-descr div', $row), 150);
		});
}