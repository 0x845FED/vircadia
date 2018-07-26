//
//  TestRailInterface.cpp
//
//  Created by Nissim Hadar on 6 Jul 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestRailInterface.h"
#include "Test.h"

#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

void TestRailInterface::createTestSuitePython(const QString& testDirectory,
                                           const QString& outputDirectory,
                                           const QString& user,
                                           const QString& branch) {
 }

 void TestRailInterface::createTestSuiteXML(const QString& testDirectory,
                                           const QString& outputDirectory,
                                           const QString& user,
                                           const QString& branch) {
    QDomProcessingInstruction instruction = document.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
    document.appendChild(instruction);

    // We create a single section, within sections
    QDomElement root = document.createElement("sections");
    document.appendChild(root);

    QDomElement topLevelSection = document.createElement("section");

    QDomElement suiteName = document.createElement("name");
    suiteName.appendChild(document.createTextNode("Test Suite - " + QDateTime::currentDateTime().toString()));
    topLevelSection.appendChild(suiteName);

    QDomElement secondLevelSections = document.createElement("sections");
    topLevelSection.appendChild(processDirectory(testDirectory, user, branch, secondLevelSections));

    topLevelSection.appendChild(secondLevelSections);
    root.appendChild(topLevelSection);

    // Write to file
    const QString testRailsFilename{ outputDirectory  + "/TestRailSuite.xml" };
    QFile file(testRailsFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Could not create XML file");
        exit(-1);
    }

    QTextStream stream(&file);
    stream << document.toString();

    file.close();

    QMessageBox::information(0, "Success", "TestRail XML file has been created");
}

QDomElement TestRailInterface::processDirectory(const QString& directory, const QString& user, const QString& branch, const QDomElement& element) {
    QDomElement result = element;

    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
         QString nextDirectory = it.next();

       // The object name appears after the last slash (we are assured there is at least 1).
        QString objectName = nextDirectory.right(nextDirectory.length() - nextDirectory.lastIndexOf("/") - 1);

        // Only process directories
        if (Test::isAValidDirectory(nextDirectory)) {
            // Ignore the utils and preformance directories
            if (nextDirectory.right(QString("utils").length()) == "utils" || nextDirectory.right(QString("performance").length()) == "performance") {
                continue;
            }

            // Create a section and process it

            QDomElement sectionElement = document.createElement("section");

            QDomElement sectionElementName = document.createElement("name");
            sectionElementName.appendChild(document.createTextNode(objectName));
            sectionElement.appendChild(sectionElementName);

            QDomElement testsElement = document.createElement("sections");
            sectionElement.appendChild(processDirectory(nextDirectory, user, branch, testsElement));

            result.appendChild(sectionElement);
        } else if (objectName == "test.js" || objectName == "testStory.js") {
            QDomElement sectionElement = document.createElement("section");
            QDomElement sectionElementName = document.createElement("name");
            sectionElementName.appendChild(document.createTextNode("all"));
            sectionElement.appendChild(sectionElementName);
            sectionElement.appendChild(processTest(nextDirectory, objectName, user, branch, document.createElement("cases")));
            result.appendChild(sectionElement);
        }
    }

    return result;
}

QDomElement TestRailInterface::processTest(const QString& fullDirectory, const QString& test, const QString& user, const QString& branch, const QDomElement& element) {
    QDomElement result = element;
   
    QDomElement caseElement = document.createElement("case");

    caseElement.appendChild(document.createElement("id"));

    // The name of the test is derived from the full path.
    // The first term is the first word after "tests"
    // The last word is the penultimate word
    QStringList words = fullDirectory.split('/');
    int i = 0;
    while (words[i] != "tests") {
        ++i;
        if (i >= words.length() - 1) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Folder \"tests\" not found  in " + fullDirectory);
            exit(-1);
        }
    }

    ++i;
    QString title{ words[i] };
    for (++i; i < words.length() - 1; ++i) {
        title += " / " + words[i];
    }

    QDomElement titleElement = document.createElement("title");
    titleElement.appendChild(document.createTextNode(title));
    caseElement.appendChild(titleElement);

    QDomElement templateElement = document.createElement("template");
    templateElement.appendChild(document.createTextNode("Test Case (Steps)"));
    caseElement.appendChild(templateElement);

    QDomElement typeElement = document.createElement("type");
    typeElement.appendChild(document.createTextNode("3 - Regression"));
    caseElement.appendChild(typeElement);

    QDomElement priorityElement = document.createElement("priority");
    priorityElement.appendChild(document.createTextNode("Medium"));
    caseElement.appendChild(priorityElement);

    QDomElement estimateElementName = document.createElement("estimate");
    estimateElementName.appendChild(document.createTextNode("60"));
    caseElement.appendChild(estimateElementName);

    caseElement.appendChild(document.createElement("references"));

    QDomElement customElement = document.createElement("custom");

    QDomElement tester_countElement = document.createElement("tester_count");
    tester_countElement.appendChild(document.createTextNode("1"));
    customElement.appendChild(tester_countElement);

    QDomElement domain_bot_loadElement = document.createElement("domain_bot_load");
    QDomElement domain_bot_loadElementId = document.createElement("id");
    domain_bot_loadElementId.appendChild(document.createTextNode("1"));
    domain_bot_loadElement.appendChild(domain_bot_loadElementId);
    QDomElement domain_bot_loadElementValue = document.createElement("value");
    domain_bot_loadElementValue.appendChild(document.createTextNode(" Without Bots (hifiqa-rc / hifi-qa-stable / hifiqa-master)"));
    domain_bot_loadElement.appendChild(domain_bot_loadElementValue);
    customElement.appendChild(domain_bot_loadElement);

    QDomElement automation_typeElement = document.createElement("automation_type");
    QDomElement automation_typeElementId = document.createElement("id");
    automation_typeElementId.appendChild(document.createTextNode("0"));
    automation_typeElement.appendChild(automation_typeElementId);
    QDomElement automation_typeElementValue = document.createElement("value");
    automation_typeElementValue.appendChild(document.createTextNode("None"));
    automation_typeElement.appendChild(automation_typeElementValue);
    customElement.appendChild(automation_typeElement);

    QDomElement added_to_releaseElement = document.createElement("added_to_release");
    QDomElement added_to_releaseElementId = document.createElement("id");
    added_to_releaseElementId.appendChild(document.createTextNode("4"));
    added_to_releaseElement.appendChild(added_to_releaseElementId);
    QDomElement added_to_releaseElementValue = document.createElement("value");
    added_to_releaseElementValue.appendChild(document.createTextNode(branch));
    added_to_releaseElement.appendChild(added_to_releaseElementValue);
    customElement.appendChild(added_to_releaseElement);

    QDomElement precondsElement = document.createElement("preconds");
    precondsElement.appendChild(document.createTextNode("Tester is in an empty region of a domain in which they have edit rights\n\n*Note: Press 'n' to advance test script"));
    customElement.appendChild(precondsElement);

    QString testMDName = QString("https://github.com/") + user + "/hifi_tests/blob/" + branch + "/tests/content/entity/light/point/create/test.md";

    QDomElement steps_seperatedElement = document.createElement("steps_separated");
    QDomElement stepElement = document.createElement("step");
    QDomElement stepIndexElement = document.createElement("index");
    stepIndexElement.appendChild(document.createTextNode("1"));
    stepElement.appendChild(stepIndexElement);
    QDomElement stepContentElement = document.createElement("content");
    stepContentElement.appendChild(document.createTextNode(QString("Execute instructions in [THIS TEST](") + testMDName + ")"));
    stepElement.appendChild(stepContentElement);
    QDomElement stepExpectedElement = document.createElement("expected");
    stepExpectedElement.appendChild(document.createTextNode("Refer to the expected result in the linked description."));
    stepElement.appendChild(stepExpectedElement);
    steps_seperatedElement.appendChild(stepElement);
    customElement.appendChild(steps_seperatedElement);

    QDomElement notesElement = document.createElement("notes");
    notesElement.appendChild(document.createTextNode(testMDName));
    customElement.appendChild(notesElement);

    caseElement.appendChild(customElement);

    result.appendChild(caseElement);
 
    return result;
}