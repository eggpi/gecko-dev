var okCallback = 0;
var pickerID = null;
var preselectedFolderURI = null;
var dualUseFolders = true;
var foldersOnly = 0;

function newFolderNameOnLoad(pickerDOMID)
{
	pickerID = pickerDOMID;

	doSetOKCancel(newFolderNameOKButtonCallback, newFolderNameCancelButtonCallback);

	// look in arguments[0] for parameters
	if (window.arguments && window.arguments[0]) {
		if ( window.arguments[0].title ) {
			// dump("title = " + window.arguments[0].title + "\n");
			top.window.title = window.arguments[0].title;
		}

		dualUseFolders = window.arguments[0].dualUseFolders;
		
		if ( window.arguments[0].okCallback ) {
			top.okCallback = window.arguments[0].okCallback;
		}
	}
	
	// pre select the folderPicker, based on what they selected in the folder pane
	if (window.arguments[0].preselectedURI) {
		try {
			dump("pick this one: " + window.arguments[0].preselectedURI + "\n");
		}
		catch (ex) {
			dump("failed to preflight the folderPicker thing.\n");
		}
	}
	else {
		dump("passed null for preselectedURI, do nothing\n");
	}
	var picker = document.getElementById(pickerID);
	if(picker)
		MsgFolderPickerOnLoad(pickerID);
	if (!dualUseFolders)
	{
		var newFolderTypeBox = document.getElementById("newFolderTypeBox");
		if (newFolderTypeBox)
		{
			newFolderTypeBox.setAttribute("hidden", "false");
			window.sizeToContent();
		}
	}
	moveToAlertPosition();
}

function newFolderNameOKButtonCallback()
{
	if ( top.okCallback )
	{
		var name = document.getElementById("name").value;
		var picker = document.getElementById(pickerID);
		var	uri = picker.getAttribute("uri");

		// make sure we have a valid name and parent uri	
		if (name.length <= 0 || uri.length <=0)
			return false;

		//dump("uri,name in callback = " + uri + "," + name + "\n");

		if (!dualUseFolders && foldersOnly && name.charAt(name.length-1) != "/")
		{
			top.okCallback(name + "/", uri);
		}
		else
		{
			top.okCallback(name, uri);
		}
	}
	
	return true;
}

function newFolderNameCancelButtonCallback()
{
	// close the window
	dump("in newFolderNameCancelButtonCallback\n");
	return true;
}


function renameFolderNameOnLoad(pickerDOMID)
{
	doSetOKCancel(renameFolderNameOKButtonCallback, renameFolderNameCancelButtonCallback);
	moveToAlertPosition();

	// look in arguments[0] for parameters
	if (window.arguments && window.arguments[0]) {
		if ( window.arguments[0].title ) {
			// dump("title = " + window.arguments[0].title + "\n");
			top.window.title = window.arguments[0].title;
		}
		
		if ( window.arguments[0].okCallback ) {
			top.okCallback = window.arguments[0].okCallback;
		}
	}
	
	// pre select the folderPicker, based on what they selected in the folder pane
	if (window.arguments[0].preselectedURI) {
		try {
			preselectedFolderURI = window.arguments[0].preselectedURI;
			dump("pick this one: " + window.arguments[0].preselectedURI + "\n");
		}
		catch (ex) {
			dump("failed to preflight the folderPicker thing.\n");
		}
	}
	else {
		dump("passed null for preselectedURI, do nothing\n");
	}

	if(window.arguments[0].name)
	{
		var name = document.getElementById("name");
		name.value = window.arguments[0].name;
		name.setSelectionRange(0,-1);
		name.focusTextField();

	}
}


function renameFolderNameOKButtonCallback()
{
	if ( top.okCallback )
	{
		var name = document.getElementById("name").value;
		var	uri = preselectedFolderURI;

		dump("uri,name in callback = " + uri + "," + name + "\n");
		top.okCallback(name, uri);
	}
	
	return true;
}

function renameFolderNameCancelButtonCallback()
{
	// close the window
	dump("in renameFolderNameCancelButtonCallback\n");
	return true;
}

function onFoldersOnly()
{
	foldersOnly = 1;
}

function onMessagesOnly()
{
	foldersOnly = 0;
}
