function createDropdown()
{
	// configurable values:
	var defaultTitle = "public/0.11.x"; // title in the dropdown for the root version of the docs - alternatively put a version from the version array as a default
	var versionArray = ["public/0.8.x", "public/0.9.x", "public/0.10.x", "public/0.11.x"]; // list of all versions in the version folder

	var ignoreDefaultInVersionFolder = true;

	//--------------------------------------
	
	// get web root path
	var script = document.getElementById('dropdownScript');
	var scriptPath = script.src;
	var currentVersionName = defaultTitle;
	var rootDir = scriptPath.substring(0, scriptPath.lastIndexOf('web/'));
	
	// figure out in which version we're currently working in
	for (var i = 0; i < versionArray.length; i++)
	{
		var currentUrl = window.location.href.toString();
		if (currentUrl.indexOf(versionArray[i]) > 0)
		{
			currentVersionName = versionArray[i];			
			break;
		}
	}
		
	// create dropdown button
	var versionDropDiv = document.getElementById('versionDropdown');
	var btn = document.createElement('button');
	btn.className = "dropbtn";
	var buttonName = "Version - " + currentVersionName;
	var btnText = document.createTextNode(buttonName);
	btn.appendChild(btnText);
	var innerDiv = document.createElement('div');
	innerDiv.className = "version-dropdown-content";
	versionDropDiv.appendChild(btn);
	versionDropDiv.appendChild(innerDiv);
	
	var isDefaultInVersionFolder = false;
	// create version entries
	for (i = 0; i<versionArray.length; i++)
	{
		if (versionArray[i] != currentVersionName)
		{
			if (versionArray[i].localeCompare(defaultTitle) == 0)
			{
				if (ignoreDefaultInVersionFolder == false)
				{
					// remember if our current version is also the default and replaces the root folder entry
					isDefaultInVersionFolder = true;
					createEntry(innerDiv, versionArray[i], rootDir+"version/"+versionArray[i]+"/README.html", false);
				}
				else
				{
					// skip processing this entry - we'll add the root folder instead for this version
				}
			}
			else
			{
				createEntry(innerDiv, versionArray[i], rootDir+"version/"+versionArray[i]+"/README.html", false);
			}
		}
	}
	
	// create default entry
	if (currentVersionName != defaultTitle && isDefaultInVersionFolder == false)
	{
		createEntry(innerDiv, defaultTitle, rootDir+"README.html", true);
	}
	
}

function createEntry(attachTo, name, url, prepend)
{
	var a = document.createElement('a');
	var linkText = document.createTextNode(name);
	a.appendChild(linkText);
	a.href = url;
	a.title = name;
	if (prepend == true)
	{
		attachTo.prepend(a);
	}
	else
	{
		attachTo.appendChild(a);
	}
}

createDropdown();